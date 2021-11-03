#define NAPI_VERSION 4
#include <node_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <iostream>
// #include <fstream>
using namespace std;

char* parseStringArg(napi_env env, napi_value arg) {
  char* str;
  size_t str_size;
  napi_get_value_string_utf8(env, arg, NULL, 0, &str_size);
  str_size += 1;

  str = (char*)calloc(str_size + 1, sizeof(char));
  size_t str_size_read;
  napi_get_value_string_utf8(env, arg, str, str_size, &str_size_read);
  return str;
}

napi_value encrypt(napi_env env, napi_callback_info info) {
  napi_value argv[2];
  size_t argc = 2;
  napi_value thisArg;
  napi_get_cb_info(env, info, &argc, argv, &thisArg, NULL);

  char* user_name = parseStringArg(env, argv[0]);
  char prefix[] = "Hello ";
  strcat(prefix, user_name);

  napi_value return_value;
  napi_create_string_utf8(env, prefix, NAPI_AUTO_LENGTH, &return_value);
  return return_value;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_value fn;
  napi_create_function(env, NULL, 0, encrypt, NULL, &fn);
  napi_set_named_property(env, exports, "encrypt", fn);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
