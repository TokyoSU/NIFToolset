using System.Runtime.InteropServices;
using System.Text;

namespace NIFToolset.Managed;

public sealed class NifNativeException : Exception
{
    public NifNativeException(NifResult result, string message)
        : base(message)
    {
        Result = result;
    }

    public NifResult Result { get; }
}

public static class NifNative
{
    static NifNative()
    {
        AbiValidation.Validate();
    }

    /// <summary>Validates managed/native structure layouts before interop use.</summary>
    public static void ValidateAbi() => AbiValidation.Validate();

    public static Version Version => new(
        checked((int)NativeMethods.NIF_GetVersionMajor()),
        checked((int)NativeMethods.NIF_GetVersionMinor()),
        checked((int)NativeMethods.NIF_GetVersionPatch()));

    public static string VersionString => Utf8String(NativeMethods.NIF_GetVersionString());

    public static bool IsRuntimeAvailable => NativeMethods.NIF_IsRuntimeAvailable() != 0;

    public static int ToNativeBool(bool value) => value ? 1 : 0;

    public static bool FromNativeBool(int value) => value != 0;

    internal static void EnsureInitialized()
    {
        // Calling this method triggers the static constructor.
    }

    public static NifResult LastErrorCode => NativeMethods.NIF_GetLastErrorCode();

    public static string LastErrorMessage
    {
        get
        {
            nuint byteLength = NativeMethods.NIF_GetLastErrorMessageLength();
            if (byteLength == 0)
            {
                return string.Empty;
            }

            // Native length excludes the null terminator.
            nuint bufferSize = checked(byteLength + 1);
            nint buffer = Marshal.AllocHGlobal(checked((nint)bufferSize));
            try
            {
                if (NativeMethods.NIF_CopyLastErrorMessage(buffer, bufferSize) == 0)
                {
                    return string.Empty;
                }

                byte[] bytes = new byte[checked((int)byteLength)];
                Marshal.Copy(buffer, bytes, 0, bytes.Length);
                return Encoding.UTF8.GetString(bytes);
            }
            finally
            {
                Marshal.FreeHGlobal(buffer);
            }
        }
    }

    public static void ClearLastError() => NativeMethods.NIF_ClearLastError();

    public static void ThrowLastError(string operation)
    {
        NifResult result = LastErrorCode;
        string message = LastErrorMessage;
        if (string.IsNullOrWhiteSpace(message))
        {
            message = $"{operation} failed with {result}.";
        }
        else
        {
            message = $"{operation} failed: {message}";
        }

        throw new NifNativeException(result, message);
    }

    /// <summary>
    /// Copies a null-terminated UTF-8 string returned by the native API.
    /// The pointer is never freed by managed code.
    /// </summary>
    public static string Utf8String(nint nativeString)
    {
        if (nativeString == 0)
        {
            return string.Empty;
        }

        nuint required = NativeMethods.NIF_CopyString(nativeString, 0, 0);
        if (required <= 1)
        {
            return string.Empty;
        }

        nint buffer = Marshal.AllocHGlobal(checked((nint)required));
        try
        {
            NativeMethods.NIF_CopyString(nativeString, buffer, required);
            int payloadLength = checked((int)required - 1);
            byte[] bytes = new byte[payloadLength];
            Marshal.Copy(buffer, bytes, 0, payloadLength);
            return Encoding.UTF8.GetString(bytes);
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }

    internal static string CopyUtf8String(Func<nint, nuint, nuint> copy)
    {
        nuint required = copy(0, 0);
        if (required <= 1)
        {
            return string.Empty;
        }

        nint buffer = Marshal.AllocHGlobal(checked((nint)required));
        try
        {
            copy(buffer, required);
            int payloadLength = checked((int)required - 1);
            byte[] bytes = new byte[payloadLength];
            Marshal.Copy(buffer, bytes, 0, payloadLength);
            return Encoding.UTF8.GetString(bytes);
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }
}
