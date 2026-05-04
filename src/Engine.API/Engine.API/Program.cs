// See https://aka.ms/new-console-template for more information
using System.Runtime.InteropServices;

Console.WriteLine("Hello, World!");
[DllImport("libEngine.Compute")] // .NET är smart nog att själv lägga till .dll eller .so beroende på OS
static extern float AddValues(float a, float b);


[DllImport("libEngine.Compute")]
static extern float SubtractValues(float a, float b);

float addedValue = AddValues(3, 3);

Console.WriteLine(addedValue);

float subtractedValue = SubtractValues(20, 3);

Console.WriteLine(subtractedValue);