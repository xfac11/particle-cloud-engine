using System;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace Engine.API;

internal enum EngineStatus : uint
{
    Ok = 0,
    InvalidArgument = 1,
    OutOfMemory = 2,
    InitializationFailed = 3,
    InvalidState = 4,
    StepFailed = 5,
    NotImplemented = 6,
    InternalError = 7
}

[StructLayout(LayoutKind.Sequential)]
internal struct EngineCreateOptions
{
    public uint StructSize;
    public uint InitialParticleCount;
}

internal static partial class NativeMethods
{
    internal const string LibraryName = "Engine.Compute";

    [LibraryImport(LibraryName, EntryPoint = "EngineCreate")]
    internal static partial EngineStatus EngineCreate(
        in EngineCreateOptions options,
        out IntPtr outHandle);

    [LibraryImport(LibraryName, EntryPoint = "EngineStep")]
    internal static partial EngineStatus EngineStep(SafeEngineHandle handle);

    [LibraryImport(LibraryName, EntryPoint = "EngineDestroy")]
    internal static partial EngineStatus EngineDestroy(IntPtr handle);

    [LibraryImport(LibraryName, EntryPoint = "EngineGetLastError")]
    internal static partial EngineStatus EngineGetLastError(
        IntPtr buffer,
        uint bufferCapacity,
        out uint requiredCapacity);
}

internal sealed class SafeEngineHandle : SafeHandleZeroOrMinusOneIsInvalid
{
    private SafeEngineHandle() : base(ownsHandle: true) { }

    internal SafeEngineHandle(IntPtr handle) : base(ownsHandle: true)
    {
        SetHandle(handle);
    }

    protected override bool ReleaseHandle()
    {
        return NativeMethods.EngineDestroy(handle) == EngineStatus.Ok;
    }
}

internal sealed class ParticleEngineWrapper : IDisposable
{
    private SafeEngineHandle? _handle;

    internal ParticleEngineWrapper(uint initialParticleCount)
    {
        var options = new EngineCreateOptions
        {
            StructSize = (uint)Marshal.SizeOf<EngineCreateOptions>(),
            InitialParticleCount = initialParticleCount
        };

        EngineStatus status = NativeMethods.EngineCreate(in options, out IntPtr nativeHandle);
        if (status != EngineStatus.Ok)
        {
            throw CreateNativeException(status);
        }

        if (nativeHandle == IntPtr.Zero)
        {
            throw new InvalidOperationException("Native EngineCreate succeeded without returning a handle.");
        }

        _handle = new SafeEngineHandle(nativeHandle);
    }

    // Advances one logical fixed tick. Wall-clock scheduling belongs to the caller.
    internal void Tick()
    {
        SafeEngineHandle handle = _handle
            ?? throw new ObjectDisposedException(nameof(ParticleEngineWrapper));

        EngineStatus status = NativeMethods.EngineStep(handle);
        if (status != EngineStatus.Ok)
        {
            throw CreateNativeException(status);
        }
    }

    public void Dispose()
    {
        _handle?.Dispose();
        _handle = null;
        GC.SuppressFinalize(this);
    }

    private static Exception CreateNativeException(EngineStatus status)
    {
        string? diagnostic = TryGetNativeDiagnostic();
        string message = string.IsNullOrWhiteSpace(diagnostic)
            ? $"Native engine call failed with status {status}."
            : $"Native engine call failed with status {status}: {diagnostic}";
        return status == EngineStatus.NotImplemented
            ? new NotSupportedException(message)
            : new InvalidOperationException(message);
    }

    private static string? TryGetNativeDiagnostic()
    {
        if (NativeMethods.EngineGetLastError(IntPtr.Zero, 0, out uint required) != EngineStatus.Ok ||
            required == 0)
        {
            return null;
        }

        IntPtr buffer = Marshal.AllocHGlobal(checked((int)required));
        try
        {
            if (NativeMethods.EngineGetLastError(buffer, required, out _) != EngineStatus.Ok)
            {
                return null;
            }

            return Marshal.PtrToStringUTF8(buffer);
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }
}
