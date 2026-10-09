using System.Globalization;
using DotNative.Plugins;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.DependencyInjection.Extensions;

namespace DotNative.NativeDatePicker;

public interface INativeDatePicker
{
    /// <summary>Returns the selected date, or null when the dialog is dismissed.</summary>
    Task<DateOnly?> ShowAsync(
        DatePickerOptions? options = null,
        CancellationToken cancellationToken = default
    );
}

public sealed record DatePickerOptions
{
    public DateOnly MinimumDate { get; init; } = new(1900, 1, 1);
    public DateOnly MaximumDate { get; init; } = new(2100, 12, 31);
    public DateOnly? InitialDate { get; init; }
    public string Title { get; init; } = "Choose date";
    public string ConfirmText { get; init; } = "OK";
    public string CancelText { get; init; } = "Cancel";
}

internal sealed class ChannelNativeDatePicker(
    IPlatformChannels channels,
    DatePickerOptions defaults
) : INativeDatePicker
{
    private readonly MethodChannel channel = channels.Get("dotnative.nativedatepicker");
    private readonly SemaphoreSlim gate = new(1, 1);

    public async Task<DateOnly?> ShowAsync(
        DatePickerOptions? options = null,
        CancellationToken cancellationToken = default
    )
    {
        var value = options ?? defaults;
        if (value.MinimumDate > value.MaximumDate)
            throw new ArgumentException(
                "MinimumDate must not exceed MaximumDate.",
                nameof(options)
            );
        if (value.MaximumDate.Year - value.MinimumDate.Year > 500)
            throw new ArgumentException(
                "The supported range is at most 500 years.",
                nameof(options)
            );
        var date = value.InitialDate ?? DateOnly.FromDateTime(DateTime.Today);
        if (date < value.MinimumDate)
            date = value.MinimumDate;
        if (date > value.MaximumDate)
            date = value.MaximumDate;
        await gate.WaitAsync(cancellationToken).ConfigureAwait(false);
        try
        {
            var result = await channel
                .InvokeAsync(
                    "show",
                    new Dictionary<string, object?>
                    {
                        ["minimumDate"] = value.MinimumDate.ToString(
                            "yyyy-MM-dd",
                            CultureInfo.InvariantCulture
                        ),
                        ["maximumDate"] = value.MaximumDate.ToString(
                            "yyyy-MM-dd",
                            CultureInfo.InvariantCulture
                        ),
                        ["initialDate"] = date.ToString("yyyy-MM-dd", CultureInfo.InvariantCulture),
                        ["title"] = value.Title,
                        ["confirmText"] = value.ConfirmText,
                        ["cancelText"] = value.CancelText,
                    },
                    cancellationToken
                )
                .ConfigureAwait(false);
            if (result is null)
                return null;
            if (
                result is not string text
                || !DateOnly.TryParseExact(
                    text,
                    "yyyy-MM-dd",
                    CultureInfo.InvariantCulture,
                    DateTimeStyles.None,
                    out var selected
                )
                || selected < value.MinimumDate
                || selected > value.MaximumDate
            )
                throw new InvalidDataException("The native picker returned an invalid date.");
            return selected;
        }
        finally
        {
            try
            {
                if (cancellationToken.IsCancellationRequested)
                    await channel
                        .InvokeAsync("cancel", cancellationToken: CancellationToken.None)
                        .ConfigureAwait(false);
            }
            finally
            {
                gate.Release();
            }
        }
    }
}

public static class NativeDatePickerServices
{
    public static IServiceCollection AddNativeDatePicker(
        this IServiceCollection services,
        DatePickerOptions? options = null
    )
    {
        services.TryAddSingleton<INativeDatePicker>(p => new ChannelNativeDatePicker(
            p.GetRequiredService<IPlatformChannels>(),
            options ?? new()
        ));
        return services;
    }
}

public static class NativeDatePickerServiceProviderExtensions
{
#if NET10_0_OR_GREATER
    extension(IServiceProvider services)
    {
        public INativeDatePicker NativeDatePicker =>
            services.GetRequiredService<INativeDatePicker>();
    }
#else
    public static INativeDatePicker NativeDatePicker(this IServiceProvider services) =>
        services.GetRequiredService<INativeDatePicker>();
#endif
}
