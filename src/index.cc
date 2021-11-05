#define NAPI_VERSION 4
#include <node_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <fstream>
using namespace std;

static const string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];
 
  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;
 
      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }
 
  if (i) {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';
 
    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;
 
    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];
 
    while((i++ < 3))
      ret += '=';
  }
  return ret;
}

uint64_t timeSinceEpochMillisec() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
    .count();
}

// BKDR Hash Function
unsigned int BKDRHash(const char *str)
{
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;

	while (*str)
	{
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF);
}

char* parse_string(napi_env env, napi_value arg) {
  char* str;
  size_t str_size;
  napi_get_value_string_utf8(env, arg, NULL, 0, &str_size);
  str_size += 1;

  str = (char*)calloc(str_size, sizeof(char));
  size_t str_size_read;
  napi_get_value_string_utf8(env, arg, str, str_size, &str_size_read);
  return str;
}

string readFileContent(const char* filePath) {
  ifstream ifs(filePath);
  if (ifs) {
    string content( (istreambuf_iterator<char>(ifs)),
                    (istreambuf_iterator<char>())
    );
    return content;
  }
  return string();
}

/**
 * Error: 1
    at <anonymous>:4:9
    at Object.<anonymous> (/Users/dickeylth/dev/github/require-verify/test/index.js:2:13)
    at Module._compile (internal/modules/cjs/loader.js:1085:14)
    at Object.Module._extensions..js (internal/modules/cjs/loader.js:1114:10)
    at Module.load (internal/modules/cjs/loader.js:950:32)
    at Function.Module._load (internal/modules/cjs/loader.js:790:12)
    at Function.executeUserEntryPoint [as runMain] (internal/modules/run_main.js:76:12)
    at internal/main/run_main_module.js:17:47 
 */
string parse_require_path(napi_env env) {
  string script = R""""(
let stack;
try {
  throw new Error(1);
} catch(e){
  stack = e.stack;
};
let filename = stack.split('\n')[2].split('(')[1].split(':')[0];
filename;
)"""";
  napi_value scriptVal;
  napi_value scriptResult;
  napi_create_string_utf8(env, script.c_str(), NAPI_AUTO_LENGTH, &scriptVal);
  napi_run_script(env, scriptVal, &scriptResult);
  string val = string(parse_string(env, scriptResult));
  // cout << "parse_require_path: " + val << endl;
  return val;
}

string parse_pkg_json(napi_env env, string filename) {
  string pkg_file_path;
  string pkg_content;
  size_t pos = 0;
  while (pos != string::npos && filename.size() > 0) {
    pos = filename.find_last_of("/");
    filename = filename.substr(0, pos);
    // cout << filename + "/package.json" << endl;
    pkg_file_path = filename + "/package.json";
    pkg_content = readFileContent(pkg_file_path.c_str());
    if (pkg_content.size() > 0) {
      // cout << "file exists: " + pkg_file_path << endl;
      break;
    }
  }

  napi_value pkgParseScript;
  string json_parse_script = "let pkg = JSON.parse(`" + pkg_content + "`);pkg.name + '@' + pkg.version";
  // cout << json_parse_script << endl;
  napi_create_string_utf8(env, json_parse_script.c_str(), NAPI_AUTO_LENGTH, &pkgParseScript);
  napi_value pkgMeta;
  napi_run_script(env, pkgParseScript, &pkgMeta);

  return string(parse_string(env, pkgMeta));
}

// 非常简单的字符串加密函数，注意可能输出控制字符
string simple_string_encrypt(string str, string key) {
  const size_t key_size = key.size();
  if (key_size == 0) return str;
  for (string::size_type i = 0; i < str.size(); ++i) {
    str[i] ^= key[i % key_size];
  }
  return str;
}


napi_value get_sec_sign(napi_env env, napi_callback_info info) {
  string req_filename = parse_require_path(env);
  // cout << "req_filename:<" + req_filename + ">" << endl;

  string pkg_meta = parse_pkg_json(env, req_filename);

  string req_file_content = readFileContent(req_filename.c_str());
  unsigned int req_file_hash = BKDRHash(req_file_content.c_str());
  string req_file_hash_str = to_string(req_file_hash);

  const string ts_key = to_string(timeSinceEpochMillisec());
  const string enc_key = ENC_PUBLIC_KEY;
  // cout << "enc_key:<" + enc_key + ">" << endl;

  string enc_content = req_filename + "::" + pkg_meta 
                        + "::" + req_file_hash_str
                        + "::" + ts_key
                        + "::" + enc_key;
  // cout << "enc_content: <" + enc_content + ">" << endl;
  string base64_content = base64_encode(
    reinterpret_cast<const unsigned char*>(enc_content.c_str()),
    enc_content.length()
  );
  // cout << "encoded: " << base64_content << endl;

  string enc_result = simple_string_encrypt(base64_content, enc_key);
  string enc_base64_result = base64_encode(
    reinterpret_cast<const unsigned char*>(enc_result.c_str()),
    enc_result.length()
  );
  // cout << "enc_result: < " + enc_base64_result << endl;

  napi_value return_value;
  napi_create_string_utf8(env, enc_base64_result.c_str(), NAPI_AUTO_LENGTH, &return_value);
  return return_value;
}

napi_value string_encrypt(napi_env env, napi_callback_info info) {
  napi_value argv[2];
  size_t argc = 2;
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);

  char* str = parse_string(env, argv[0]);
  char* key = parse_string(env, argv[1]);
  string res = simple_string_encrypt(string(str), string(key));

  napi_value result;
  napi_create_string_utf8(env, res.c_str(), NAPI_AUTO_LENGTH, &result);
  return result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_value secSignFn;
  napi_create_function(env, NULL, 0, get_sec_sign, NULL, &secSignFn);
  napi_set_named_property(env, exports, "getSecSign", secSignFn);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
