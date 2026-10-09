using DotNative;
using DotNative.NativeDatePicker;

namespace PluginExample;

public static class Program
{
    public static void Main()
    {
        var builder = DotNativeApplication.CreateBuilder();
        builder.Services.AddNativeDatePicker();
        builder.Build().Run<App>();
    }
}
