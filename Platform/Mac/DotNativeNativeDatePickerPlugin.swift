import AppKit

@MainActor final class DotNativeNativeDatePickerPlugin: NSObject {
  private static var instance: DotNativeNativeDatePickerPlugin?
  static func register() { if instance == nil { instance = DotNativeNativeDatePickerPlugin() } }
  private var window: NSWindow?
  private var pending: PluginReply?
  private let picker = NSDatePicker()
  override init() {
    super.init()
    let channel = NativeChannels.channel("dotnative.nativedatepicker")
    channel.handle("show") { [self] args, reply in show(args, reply) }
    channel.handle("cancel") { [self] _, reply in
      finish(nil)
      reply.success(.null)
    }
    channel.onReset = { [self] in finish(nil) }
  }
  private func show(_ args: PluginValue, _ reply: PluginReply) {
    guard pending == nil else {
      reply.failure("picker_unavailable", "A date picker is already open")
      return
    }
    guard let parent = NativeChannels.window else {
      reply.failure("picker_unavailable", "No active window")
      return
    }
    let map = args.fields
    let formatter = DateFormatter()
    formatter.calendar = Calendar(identifier: .gregorian)
    formatter.locale = Locale(identifier: "en_US_POSIX")
    formatter.dateFormat = "yyyy-MM-dd"
    formatter.isLenient = false
    guard let minText = map["minimumDate"]?.string, let maxText = map["maximumDate"]?.string,
      let initialText = map["initialDate"]?.string, let minimum = formatter.date(from: minText),
      let maximum = formatter.date(from: maxText), let initial = formatter.date(from: initialText),
      minimum <= initial, initial <= maximum,
      let title = map["title"]?.string, let confirm = map["confirmText"]?.string,
      let cancel = map["cancelText"]?.string
    else {
      reply.failure("invalid_arguments", "Expected valid date picker options")
      return
    }
    picker.datePickerStyle = .clockAndCalendar
    picker.datePickerElements = [.yearMonthDay]
    picker.calendar = Calendar(identifier: .gregorian)
    picker.minDate = minimum
    picker.maxDate = maximum
    picker.dateValue = initial
    picker.sizeToFit()
    let calendarSize = picker.fittingSize
    let width = max(280, calendarSize.width + 48)
    let height = calendarSize.height + 88
    let sheet = NSWindow(
      contentRect: NSRect(x: 0, y: 0, width: width, height: height), styleMask: [.titled],
      backing: .buffered, defer: false)
    sheet.title = title
    picker.frame = NSRect(
      x: (width - calendarSize.width) / 2, y: 64,
      width: calendarSize.width, height: calendarSize.height)
    sheet.contentView?.addSubview(picker)
    let saveButton = NSButton(title: confirm, target: self, action: #selector(confirmDate))
    saveButton.bezelStyle = .rounded
    saveButton.keyEquivalent = "\r"
    saveButton.frame = NSRect(x: width - 140, y: 20, width: 116, height: 32)
    let cancelButton = NSButton(title: cancel, target: self, action: #selector(cancelDate))
    cancelButton.bezelStyle = .rounded
    cancelButton.keyEquivalent = "\u{1b}"
    cancelButton.frame = NSRect(x: width - 264, y: 20, width: 116, height: 32)
    sheet.contentView?.addSubview(saveButton)
    sheet.contentView?.addSubview(cancelButton)
    window = sheet
    pending = reply
    reply.onCancel = { [weak self] in self?.finish(nil) }
    parent.beginSheet(sheet)
  }
  @objc private func confirmDate() {
    let formatter = DateFormatter()
    formatter.calendar = Calendar(identifier: .gregorian)
    formatter.locale = Locale(identifier: "en_US_POSIX")
    formatter.dateFormat = "yyyy-MM-dd"
    finish(formatter.string(from: picker.dateValue))
  }
  @objc private func cancelDate() { finish(nil) }
  private func finish(_ date: String?) {
    let reply = pending
    pending = nil
    if let window {
      window.sheetParent?.endSheet(window)
      window.orderOut(nil)
    }
    window = nil
    reply?.success(date.map { .string($0) } ?? .null)
  }
}
