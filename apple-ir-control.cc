//
// apple-ir-control
// Copyright (c) 2016, Robert Sesek <https://www.bluestatic.org>
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or any later version.
//

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/IOKitLib.h>
#include <unistd.h>
#include <vector>

const CFStringRef kPrefDomain = CFSTR("com.apple.driver.AppleIRController");
const CFStringRef kPrefEnabled = CFSTR("DeviceEnabled");

#if NDEBUG
const bool VERBOSE = false;
#else
const bool VERBOSE = true;
#endif

#define LOCATION_LINE(x) #x
#define LOCATION(line) __FILE__ ":" LOCATION_LINE(line)

#define LOG(msg, ...) do { \
    if (VERBOSE) { \
      printf("[" LOCATION(__LINE__) "] " msg "\n", ##__VA_ARGS__); \
    } \
  } while(0)
#define ERROR(msg, ...) fprintf(stderr, "[" LOCATION(__LINE__) "] " msg "\n", ##__VA_ARGS__)

class ScopedIOHIDManager {
 public:
  ScopedIOHIDManager()
      : manager_(IOHIDManagerCreate(nullptr, kIOHIDManagerOptionNone)) {}

  ~ScopedIOHIDManager() {
    IOHIDManagerClose(manager_, kIOHIDManagerOptionNone);
  }

  ScopedIOHIDManager(const ScopedIOHIDManager&) = delete;

  IOHIDManagerRef get() { return manager_; }

 private:
  IOHIDManagerRef manager_;
};

template <typename T>
class ScopedCFTypeRef {
 public:
  ScopedCFTypeRef(T t = nullptr) : t_(t) {}
  ~ScopedCFTypeRef() {
    if (t_) {
      CFRelease(t_);
    }
  }

  ScopedCFTypeRef(const ScopedCFTypeRef<T>&) = delete;

  operator bool() { return t_ != nullptr; }

  T get() { return t_; }

  T* pointer_to() { return &t_; }

 private:
  T t_;
};

bool IsIRAvailable() {
  ScopedIOHIDManager manager;
  IOHIDManagerSetDeviceMatching(manager.get(), nullptr);
  ScopedCFTypeRef<CFSetRef> devices(IOHIDManagerCopyDevices(manager.get()));
  if (!devices) {
    ERROR("Failed to IOHIDManagerCopyDevices");
    return false;
  }

  std::vector<void*> devices_array(CFSetGetCount(devices.get()), nullptr);
  CFSetGetValues(devices.get(), const_cast<const void**>(devices_array.data()));
  for (const auto& device : devices_array) {
    CFTypeRef prop =
        IOHIDDeviceGetProperty(reinterpret_cast<IOHIDDeviceRef>(device),
                               CFSTR("HIDRemoteControl"));
    if (prop) {
      if (VERBOSE) {
        LOG("Located HIDRemoteControl:");
        CFShow(device);
      }
      return true;
    }
  }
  return false;
}

const char* GetBooleanDescription(CFTypeRef boolean) {
  if (CFGetTypeID(boolean) != CFBooleanGetTypeID()) {
    ERROR("Unexpected non-boolean CFTypeRef");
    abort();
  }
  return CFBooleanGetValue(static_cast<CFBooleanRef>(boolean)) ? "on" : "off";
}

bool SynchronizePrefs() {
  bool rv = CFPreferencesSynchronize(kPrefDomain, kCFPreferencesAnyUser,
      kCFPreferencesCurrentHost);
  if (!rv) {
    ERROR("Failed to CFPreferencesSynchronize");
  }
  return rv;
}

bool CreateIOServiceIterator(io_iterator_t* iterator) {
  CFMutableDictionaryRef matching_dict = IOServiceMatching("AppleIRController");
  kern_return_t kr = IOServiceGetMatchingServices(
      kIOMainPortDefault, matching_dict, iterator); // Updated line
  if (kr != KERN_SUCCESS) {
    ERROR("Failed to IOServiceGetMatchingServices: 0x%x", kr);
    return false;
  }
  return true;
}

int HandleRead() {
  CFTypeRef user_prop = CFPreferencesCopyValue(kPrefEnabled, kPrefDomain,
      kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
  printf("Userspace property value: %s\n", GetBooleanDescription(user_prop));

  io_iterator_t iterator;
  if (!CreateIOServiceIterator(&iterator))
    return EXIT_FAILURE;

  io_object_t service;
  bool did_find = false;
  while ((service = IOIteratorNext(iterator))) {
    did_find = true;

    io_name_t name;
    kern_return_t kr = IORegistryEntryGetName(service, name);
    if (kr != KERN_SUCCESS) {
      ERROR("Failed to IORegistryEntryGetName: 0x%x", kr);
      continue;
    }

    LOG("Found AppleIRController: %s", name);

    ScopedCFTypeRef<CFTypeRef> device_enabled(
        IORegistryEntryCreateCFProperty(service, kPrefEnabled, nullptr, 0));
    printf("Kernel property value %s: %s\n",
        name, GetBooleanDescription(device_enabled.get()));
  }

  if (!did_find) {
    ERROR("Failed to match any AppleIRController");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int HandleWrite(bool enable) {
  if (geteuid() != 0) {
    ERROR("This operation must be performed as root");
    return EXIT_FAILURE;
  }

  const CFBooleanRef enabled_value = enable ? kCFBooleanTrue : kCFBooleanFalse;

  CFPreferencesSetValue(kPrefEnabled, enabled_value, kPrefDomain,
      kCFPreferencesAnyUser, kCFPreferencesCurrentHost);
  if (!SynchronizePrefs())
    return EXIT_FAILURE;

  io_iterator_t iterator;
  if (!CreateIOServiceIterator(&iterator))
    return EXIT_FAILURE;

  io_object_t service;
  while ((service = IOIteratorNext(iterator))) {
    io_name_t name;
    kern_return_t kr = IORegistryEntryGetName(service, name);
    if (kr != KERN_SUCCESS) {
      ERROR("Failed to IORegistryEntryGetName: 0x%x", kr);
      continue;
    }

    LOG("Setting property for %s to %d", name, enable);

    kr = IORegistryEntrySetCFProperty(service, kPrefEnabled, enabled_value);
    if (kr != KERN_SUCCESS) {
      ERROR("Failed to IORegistryEntrySetCFProperty: 0x%x", kr);
      continue;
    }
  }

  return HandleRead();
}

int main(int argc, char* argv[]) {
  if (!IsIRAvailable()) {
    ERROR("No HIDRemoteControl available");
    return EXIT_FAILURE;
  }

  if (argc == 1) {
    return HandleRead();
  } else if (argc == 2) {
    if (strcmp("on", argv[1]) == 0) {
      return HandleWrite(true);
    } else if (strcmp("off", argv[1]) == 0) {
      return HandleWrite(false);
    }
  }

  fprintf(stderr, "Usage: %s [on|off]\n", argv[0]);
  return EXIT_FAILURE;}
