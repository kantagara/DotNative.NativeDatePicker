# DotNative.NativeDatePicker

Operating-system date controls: UIDatePicker on iOS and DatePickerDialog on Android.

## Usage

```csharp
using DotNative.NativeDatePicker;

builder.Services.AddNativeDatePicker(new DatePickerOptions
{
    MinimumDate = new(1930, 1, 1),
    MaximumDate = DateOnly.FromDateTime(DateTime.Today),
    Title = "Datum rođenja",
    ConfirmText = "Sačuvaj",
    CancelText = "Otkaži",
});

// Inject INativeDatePicker into a component or service.
DateOnly? selected = await picker.ShowAsync(cancellationToken: token);
// Override registration defaults for one dialog with ShowAsync(options, token).
```

Dismissal returns null. Invalid bounds throw before opening a dialog. Dates are
calendar dates, without a time zone. Calls through one service are serialized;
only one dialog per native plugin can be open. Cancellation dismisses the dialog.

On net10.0 and later, `services.NativeDatePicker` is an extension property.
On net9.0, use `services.NativeDatePicker()`. The APIs are mutually exclusive.
The .NET 9 SDK builds only net9.0; the .NET 10 SDK also builds net10.0 with C# 14.

## Platforms

Android and iOS are implemented. Desktop implementations are not included.

## Source development

Set `DotNativeSourceRoot` in ignored `dotnative.local.props` to a DotNative checkout,
or place this repository next to `dotNative`. Otherwise the package reference is used.

```sh
dotnet build DotNative.NativeDatePicker.csproj
dotnative plugin run ios --project .
dotnative plugin run android --project .
```

The `example` app opens the real picker. Native sources use the named
`dotnative.nativedatepicker` channel and are included through buildTransitive
when packaged. No generated channel-descriptor files are required.

License: MIT.
