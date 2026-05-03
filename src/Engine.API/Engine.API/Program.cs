// See https://aka.ms/new-console-template for more information
using System.Runtime.InteropServices;

Console.WriteLine("Hello, World!");
[DllImport("Engine.Compute")] // .NET är smart nog att själv lägga till .dll eller .so beroende på OS
static extern float AddValues(float a, float b);


[DllImport("Engine.Compute")]
static extern float SubtractValues(float a, float b);

float addedValue = AddValues(3, 3);

Console.WriteLine(addedValue);

float subtractedValue = SubtractValues(20, 3);

Console.WriteLine(subtractedValue);