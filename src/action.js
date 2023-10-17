import * as core from '@actions/core';
import      fs   from 'fs-extra';
import { sep }   from 'path';

const state = {
  get isPost() {
    return !!core.getState('isPost');
  },
};

(async () => {
  console.log('env');
  console.log(process.env);
})();
