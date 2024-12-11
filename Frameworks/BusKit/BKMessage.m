#import "BKTypes.h"
#import "BKMessage.h"

static void indent(int depth)
{
  while (depth-- > 0)
    printf("   "); /* INDENT spaces. */
}

static void print_iter(DBusMessageIter *iter, dbus_bool_t literal, int depth)
{
  do {
    int type = dbus_message_iter_get_arg_type(iter);

    if (type == DBUS_TYPE_INVALID)
      break;

    indent(depth);

    switch (type) {
      case DBUS_TYPE_STRING: {
        char *val;
        dbus_message_iter_get_basic(iter, &val);
        if (!literal)
          printf("string \"");
        printf("%s", val);
        if (!literal)
          printf("\"\n");
        break;
      }

      case DBUS_TYPE_SIGNATURE: {
        char *val;
        dbus_message_iter_get_basic(iter, &val);
        if (!literal)
          printf("signature \"");
        printf("%s", val);
        if (!literal)
          printf("\"\n");
        break;
      }

      case DBUS_TYPE_OBJECT_PATH: {
        char *val;
        dbus_message_iter_get_basic(iter, &val);
        if (!literal)
          printf("object path \"");
        printf("%s", val);
        if (!literal)
          printf("\"\n");
        break;
      }

      case DBUS_TYPE_INT16: {
        dbus_int16_t val;
        dbus_message_iter_get_basic(iter, &val);
        printf("int16 %d\n", val);
        break;
      }

      case DBUS_TYPE_UINT16: {
        dbus_uint16_t val;
        dbus_message_iter_get_basic(iter, &val);
        printf("uint16 %u\n", val);
        break;
      }

      case DBUS_TYPE_INT32: {
        dbus_int32_t val;
        dbus_message_iter_get_basic(iter, &val);
        printf("int32 %d\n", val);
        break;
      }

      case DBUS_TYPE_UINT32: {
        dbus_uint32_t val;
        dbus_message_iter_get_basic(iter, &val);
        printf("uint32 %u\n", val);
        break;
      }

      case DBUS_TYPE_INT64: {
        dbus_int64_t val;
        dbus_message_iter_get_basic(iter, &val);
        printf("int64 %ld\n", val);
        break;
      }

      case DBUS_TYPE_UINT64: {
        dbus_uint64_t val;
        dbus_message_iter_get_basic(iter, &val);
        printf("uint64 %lu\n", val);
        break;
      }

      case DBUS_TYPE_DOUBLE: {
        double val;
        dbus_message_iter_get_basic(iter, &val);
        printf("double %g\n", val);
        break;
      }

      case DBUS_TYPE_BYTE: {
        unsigned char val;
        dbus_message_iter_get_basic(iter, &val);
        printf("byte %d\n", val);
        break;
      }

      case DBUS_TYPE_BOOLEAN: {
        dbus_bool_t val;
        dbus_message_iter_get_basic(iter, &val);
        printf("boolean %s\n", val ? "true" : "false");
        break;
      }

      case DBUS_TYPE_VARIANT: {
        DBusMessageIter subiter;

        dbus_message_iter_recurse(iter, &subiter);

        printf("variant ");
        print_iter(&subiter, literal, depth + 1);
        break;
      }
      case DBUS_TYPE_ARRAY: {
        int current_type;
        DBusMessageIter subiter;

        dbus_message_iter_recurse(iter, &subiter);

        current_type = dbus_message_iter_get_arg_type(&subiter);

        if (current_type == DBUS_TYPE_BYTE) {
        //   print_ay(&subiter, depth);
          break;
        }

        printf("array [\n");
        while (current_type != DBUS_TYPE_INVALID) {
          print_iter(&subiter, literal, depth + 1);

          dbus_message_iter_next(&subiter);
          current_type = dbus_message_iter_get_arg_type(&subiter);

          if (current_type != DBUS_TYPE_INVALID)
            printf(",");
        }
        indent(depth);
        printf("]\n");
        break;
      }
      case DBUS_TYPE_DICT_ENTRY: {
        DBusMessageIter subiter;

        dbus_message_iter_recurse(iter, &subiter);

        printf("dict entry(\n");
        print_iter(&subiter, literal, depth + 1);
        dbus_message_iter_next(&subiter);
        print_iter(&subiter, literal, depth + 1);
        indent(depth);
        printf(")\n");
        break;
      }

      case DBUS_TYPE_STRUCT: {
        int current_type;
        DBusMessageIter subiter;

        dbus_message_iter_recurse(iter, &subiter);

        printf("struct {\n");
        while ((current_type = dbus_message_iter_get_arg_type(&subiter)) != DBUS_TYPE_INVALID) {
          print_iter(&subiter, literal, depth + 1);
          dbus_message_iter_next(&subiter);
          if (dbus_message_iter_get_arg_type(&subiter) != DBUS_TYPE_INVALID)
            printf(",");
        }
        indent(depth);
        printf("}\n");
        break;
      }

#ifdef DBUS_UNIX
      case DBUS_TYPE_UNIX_FD: {
        int fd;
        dbus_message_iter_get_basic(iter, &fd);

        print_fd(fd, depth + 1);

        /* dbus_message_iter_get_basic() duplicated the fd, we need to
         * close it after use. The original fd will be closed when the
         * DBusMessage is released.
         */
        close(fd);

        break;
      }
#endif

      default:
        printf(" (dbus-monitor too dumb to decipher arg type '%c')\n", type);
        break;
    }
  } while (dbus_message_iter_next(iter));
}

// Is not recursive
static id getBasicType(DBusMessageIter *iter)
{
  int type = dbus_message_iter_get_arg_type(iter);

  if (type == DBUS_TYPE_INVALID) {
    return nil;
  }

  switch (type) {
    case DBUS_TYPE_STRING: {
      char *val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSString stringWithCString:val];
      break;
    }

    case DBUS_TYPE_BYTE: {
      unsigned char val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSString stringWithFormat:@"%c", val];
      break;
    }

    case DBUS_TYPE_SIGNATURE: {
      char *val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSString stringWithCString:val];
      break;
    }

    case DBUS_TYPE_OBJECT_PATH: {
      char *val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSString stringWithCString:val];
      break;
    }

    case DBUS_TYPE_INT16: {
      dbus_int16_t val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSNumber numberWithInt:val];
      break;
    }

    case DBUS_TYPE_UINT16: {
      dbus_uint16_t val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSNumber numberWithUnsignedInt:val];
      break;
    }

    case DBUS_TYPE_INT32: {
      dbus_int32_t val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSNumber numberWithInt:val];
      break;
    }

    case DBUS_TYPE_UINT32: {
      dbus_uint32_t val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSNumber numberWithUnsignedInt:val];
      break;
    }

    case DBUS_TYPE_INT64: {
      dbus_int64_t val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSNumber numberWithUnsignedInt:val];
      break;
    }

    case DBUS_TYPE_UINT64: {
      dbus_uint64_t val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSNumber numberWithUnsignedInt:val];
      break;
    }

    case DBUS_TYPE_DOUBLE: {
      double val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSNumber numberWithDouble:val];
      break;
    }

    case DBUS_TYPE_BOOLEAN: {
      dbus_bool_t val;
      dbus_message_iter_get_basic(iter, &val);
      return [NSNumber numberWithUnsignedInt:val];
      break;
    }
  }
  return nil;
}

// Is recursive. `result` have to be set at the first call to this function.
static id parseIterator(DBusMessageIter *iter, id result)
{
  do {
    int type = dbus_message_iter_get_arg_type(iter);
    id basicResult;

    if (type == DBUS_TYPE_INVALID) {
      break;
    }

    if ((basicResult = getBasicType(iter)) != nil) {
      if (result != nil) {
        [result addObject:basicResult];
      } else {
        result = basicResult;
        break;
      }
      continue;
    }

    if (type == DBUS_TYPE_VARIANT) {
      DBusMessageIter subiter;
      dbus_message_iter_recurse(iter, &subiter);
      result = parseIterator(&subiter, nil);
      break;
    } else if (type == DBUS_TYPE_ARRAY || type == DBUS_TYPE_STRUCT) {
      DBusMessageIter subiter;
      NSMutableArray *subiter_result = [NSMutableArray array];
      id object;

      // printf("array\n");
      dbus_message_iter_recurse(iter, &subiter);

      while (dbus_message_iter_get_arg_type(&subiter) != DBUS_TYPE_INVALID) {
        object = parseIterator(&subiter, nil);
        if (object) {
          [subiter_result addObject:object];
        }
        dbus_message_iter_next(&subiter);
      }
      if (result != nil) {
        [result addObject:subiter_result];
      } else {
        result = subiter_result;
        break;
      }
    } else if (type == DBUS_TYPE_DICT_ENTRY) {
      DBusMessageIter subiter;
      id key, object;

      // printf("dict_entry\n");
      dbus_message_iter_recurse(iter, &subiter);

      key = getBasicType(&subiter);
      dbus_message_iter_next(&subiter);
      object = parseIterator(&subiter, nil);
      result = @{key : object};
      break;
    } else {
      NSLog(@"BusKit error: unknown D-Bus data type '%c'", type);
    }
  } while (dbus_message_iter_next(iter));

  return result;
}

@implementation BKMessage

- (void)dealloc
{
  dbus_message_unref(dbus_message);
  [super dealloc];
}

- (instancetype)initWithObject:(const char *)object_path
                     interface:(const char *)interface_path
                        method:(const char *)method_name
                       service:(const char *)service_path
{
  [super init];

  dbus_message = dbus_message_new_method_call(NULL, object_path, interface_path, method_name);
  dbus_message_set_auto_start(dbus_message, TRUE);
  dbus_message_set_destination(dbus_message, service_path);

  return self;
}

- (void)_appendStructureArgument:(BKStructure *)argument
{
  
}

- (void)setMethodArguments:(NSArray *)args
{
  dbus_message_iter_init_append(dbus_message, &dbus_message_iterator);

  if (args != nil) {
    // Iterate trough NSArray and fill message with arguments
    NSLog(@"[BKMessage setMethodArguments:] %@", args);
    for (id argument in args) {
      if ([argument isKindOfClass:[NSString class]]) {
        const char *value = [argument cString];
        dbus_message_iter_append_basic(&dbus_message_iterator, DBUS_TYPE_STRING, &value);
      } else if ([argument isKindOfClass:[BKStructure class]]) {
        [self _appendStructureArgument:argument];
      }
    }
  }
}

- (id)sendWithConnection:(BKConnection *)connection
{
  DBusMessage *reply;
  NSMutableArray *result = nil;

  dbus_error_init(&dbus_error);
  reply = dbus_connection_send_with_reply_and_block([connection dbusConnection], dbus_message, -1,
                                                    &dbus_error);
  if (dbus_error_is_set(&dbus_error)) {
    NSLog(@"Error %s: %s\n", dbus_error.name, dbus_error.message);
    return nil;
  }
  if (reply) {
    result = [NSMutableArray new];

    dbus_message_iter_init(reply, &dbus_message_iterator);
    parseIterator(&dbus_message_iterator, result);
    dbus_message_unref(reply);
  }

  return result;
}

@end