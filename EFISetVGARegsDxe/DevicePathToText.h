CHAR16*
(EFIAPI ConvertDevicePathToText)(
  IN CONST EFI_DEVICE_PATH_PROTOCOL   *DeviceNode,
  IN BOOLEAN                          DisplayOnly,
  IN BOOLEAN                          AllowShortcuts
  );      

CHAR16*
(EFIAPI ConvertDeviceNodeToText)(
  IN CONST EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN BOOLEAN                          DisplayOnly,
  IN BOOLEAN                          AllowShortcuts
  );
