#include "notification_helper.h"
#include <Foundation/NSString.h>
#include <Foundation/NSUserNotification.h>

void notification_helper::show(const std::string& title, const std::string& message)
{
  NSUserNotification *userNotification = [[[NSUserNotification alloc] init] autorelease];
  userNotification.title = [NSString stringWithUTF8String:title.c_str()];
  userNotification.informativeText =  [NSString stringWithUTF8String:message.c_str()];

  NSUserNotificationCenter* center = [NSUserNotificationCenter defaultUserNotificationCenter];
  [center deliverNotification:userNotification];
}

