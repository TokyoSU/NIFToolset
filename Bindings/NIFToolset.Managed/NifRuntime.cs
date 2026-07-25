namespace NIFToolset.Managed;

/// <summary>
/// Reference-counted lifetime owner for the native Gamebryo runtime.
/// Keep this object alive until every native SafeHandle has been disposed.
/// </summary>
public sealed class NifRuntime : IDisposable
{
    private static readonly object SyncRoot = new();
    private static int s_referenceCount;
    private bool _disposed;

    private NifRuntime()
    {
    }

    public static bool IsInitialized => NativeMethods.NIF_System_IsInitialized() != 0;

    public static float CurrentTimeSeconds => NativeMethods.NIF_System_GetCurrentTimeInSec();

    public static NifRuntime Initialize()
    {
        NifNative.EnsureInitialized();

        lock (SyncRoot)
        {
            if (s_referenceCount == 0)
            {
                NativeMethods.NIF_System_Init();
                if (!IsInitialized)
                {
                    NifNative.ThrowLastError(nameof(Initialize));
                }
            }

            checked
            {
                s_referenceCount++;
            }

            return new NifRuntime();
        }
    }

    public static void ResetBaseTime() => NativeMethods.NIF_System_ResetBaseTime();

    public void Dispose()
    {
        lock (SyncRoot)
        {
            if (_disposed)
            {
                return;
            }

            _disposed = true;
            if (s_referenceCount > 0)
            {
                s_referenceCount--;
                if (s_referenceCount == 0 && IsInitialized)
                {
                    NativeMethods.NIF_System_Shutdown();
                }
            }
        }

        GC.SuppressFinalize(this);
    }
}
