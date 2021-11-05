const { getSecSign } = require('../index');
const sign = getSecSign();

function decodeBase64(str) {
  const s = Buffer.from(str, 'base64');
  return s.toString('utf-8');
}

function decrypt(str, key) {
  let r = "";
  for (let i = 0; i < str.length; i++) {
    r += String.fromCharCode(str[i].charCodeAt(0) ^ key[i % key.length].charCodeAt(0));
  }
  return r;
}

console.log(sign);
console.log(
  decodeBase64(decrypt(decodeBase64(sign), "test"))
);