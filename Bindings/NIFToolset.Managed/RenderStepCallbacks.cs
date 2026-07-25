using System.Runtime.InteropServices;


namespace NIFToolset.Managed;

public delegate bool NifRenderStepCallback(nint renderStep);

/// <summary>
/// Roots managed delegates and keeps the native render-step wrapper alive while callbacks are installed.
/// Dispose this object before releasing native pipeline objects.
/// </summary>
public sealed class RenderStepCallbacks : IDisposable
{
    private sealed class CallbackState
    {
        internal NifRenderStepCallback Callback { get; set; } = null!;
    }

    private static readonly NativeMethods.RenderStepCallback CallbackThunk = Invoke;

    private readonly SafeRenderStepHandle _renderStep;
    private readonly object _sync = new();
    private bool _addedRef;
    private GCHandle _preState;
    private GCHandle _postState;
    private readonly List<GCHandle> _retiredStates = new();
    private bool _disposed;

    public RenderStepCallbacks(SafeRenderStepHandle renderStep)
    {
        NifNative.EnsureInitialized();
        _renderStep = renderStep ?? throw new ArgumentNullException(nameof(renderStep));
        _renderStep.DangerousAddRef(ref _addedRef);
    }

    public void SetPre(NifRenderStepCallback? callback)
    {
        lock (_sync)
        {
            ThrowIfDisposed();
            SetCallback(callback, isPre: true);
        }
    }

    public void SetPost(NifRenderStepCallback? callback)
    {
        lock (_sync)
        {
            ThrowIfDisposed();
            SetCallback(callback, isPre: false);
        }
    }

    private void SetCallback(NifRenderStepCallback? callback, bool isPre)
    {
        GCHandle newState = default;
        try
        {
            if (callback is not null)
            {
                newState = GCHandle.Alloc(new CallbackState { Callback = callback });
            }

            if (isPre)
            {
                NativeMethods.NIF_RenderStep_SetPreCallback(
                    _renderStep,
                    callback is null ? null : CallbackThunk,
                    newState.IsAllocated ? GCHandle.ToIntPtr(newState) : 0);
                RetireState(ref _preState);
                _preState = newState;
            }
            else
            {
                NativeMethods.NIF_RenderStep_SetPostCallback(
                    _renderStep,
                    callback is null ? null : CallbackThunk,
                    newState.IsAllocated ? GCHandle.ToIntPtr(newState) : 0);
                RetireState(ref _postState);
                _postState = newState;
            }

            newState = default;
        }
        finally
        {
            FreeState(ref newState);
        }
    }

    public void Dispose()
    {
        lock (_sync)
        {
            if (_disposed)
            {
                return;
            }

            _disposed = true;
            try
            {
                NativeMethods.NIF_RenderStep_ClearCallbacks(_renderStep);
            }
            finally
            {
                FreeState(ref _preState);
                FreeState(ref _postState);
                foreach (GCHandle retiredState in _retiredStates)
                {
                    if (retiredState.IsAllocated)
                    {
                        retiredState.Free();
                    }
                }
                _retiredStates.Clear();
                if (_addedRef)
                {
                    _renderStep.DangerousRelease();
                    _addedRef = false;
                }
            }
        }

        GC.SuppressFinalize(this);
    }

    ~RenderStepCallbacks()
    {
        // The static thunk stays rooted for process lifetime. Freeing the state only happens
        // after native callback removal, so the native side cannot call a collected delegate.
        try
        {
            Dispose();
        }
        catch
        {
            // Never let process-shutdown interop failures escape a finalizer.
        }
    }

    private static int Invoke(nint renderStep, nint userData)
    {
        if (userData == 0)
        {
            return 1;
        }

        try
        {
            GCHandle handle = GCHandle.FromIntPtr(userData);
            return handle.Target is CallbackState state && state.Callback(renderStep) ? 1 : 0;
        }
        catch
        {
            // Exceptions must never cross an unmanaged callback boundary.
            return 0;
        }
    }

    private void RetireState(ref GCHandle state)
    {
        if (state.IsAllocated)
        {
            _retiredStates.Add(state);
            state = default;
        }
    }

    private static void FreeState(ref GCHandle state)
    {
        if (state.IsAllocated)
        {
            state.Free();
        }
    }

    private void ThrowIfDisposed()
    {
        if (_disposed)
        {
            throw new ObjectDisposedException(nameof(RenderStepCallbacks));
        }
    }
}
