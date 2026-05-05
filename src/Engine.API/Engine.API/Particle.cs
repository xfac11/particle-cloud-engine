using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Engine.API
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Particle
    {
        public float x, y, z;
        public float vx, vy, vz;
        public float mass;
    }
}
