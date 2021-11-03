const { encrypt } = require('node-gyp-build')(__dirname);
module.exports = (name) => {
  const result = encrypt(name, module);
  return result;
};
