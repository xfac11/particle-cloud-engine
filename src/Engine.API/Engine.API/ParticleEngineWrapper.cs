using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace Engine.API
{
    internal static partial class NativeMethods
    {
        private const string _LibraryName = "libEngine.Compute";

        [LibraryImport(_LibraryName)]
        public static partial IntPtr CreateParticleEngine();
        [LibraryImport(_LibraryName)]
        public static partial void DestroyParticleEngine(IntPtr particleEngine);
        [LibraryImport(_LibraryName)]
        public static partial void UpdateParticles(IntPtr particleEngine, float dt);

    }

    internal class ParticleEngineWrapper
    {
        private IntPtr enginieHandle;

        public void Initiate()
        {
            enginieHandle = IntPtr.Zero;
            enginieHandle = NativeMethods.CreateParticleEngine();
            if (enginieHandle == IntPtr.Zero)
            {
                throw new Exception("Could not create the particle engine");
            }

        }
        public void Tick(float dt)
        {
            NativeMethods.UpdateParticles(enginieHandle, dt);
        }
    }
}
