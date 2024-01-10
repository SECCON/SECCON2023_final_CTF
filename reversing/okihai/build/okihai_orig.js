const fs = require('fs');
const crypto = require('crypto');

const assets_dir = process.platform == "win32" ? "C:\\snapshot\\okihai\\assets\\" : "/snapshot/okihai/assets/"

const chars = [11 * 5, 50 + 2, 114, 52 - 1, 100 + 10 + 2, 109+1-1+1-1+1-1+1-1+1-1+1-1+1-1+1-1+1-1+1-1, 95+1-1+1-1+1-1+1-1, 51+1-1+1-1+1-1+1-1, 118+1-1+1-1, 51+1-1, 10 * (2 * 5), 7 * 7, 110, 11 * 5];

const xor = (buf, str) => {
    const strBuf = Buffer.from(str.repeat(Math.ceil(buf.length / str.length)));
    return Buffer.from(buf.map((b, i) => b ^ strBuf[i]));
};

const readAndXorFileSync = (path) => xor(fs.readFileSync(path), [0, 1, 5, 4, 3, 2, 6, 7, 8, 11, 10, 9, 12, 13].map(i => String.fromCharCode(chars[i])).join(''));

const keys = [...Array(1000).keys()].map(i => fs.readFileSync(assets_dir + `${i}.key`));
const iv = readAndXorFileSync(assets_dir + 'iv');
const flag = readAndXorFileSync(assets_dir + 'flag');

const encryptWithKey = (data, key) => {
    const cipher = crypto.createCipheriv(atob('YWVzL'+'TI1Ni'+atob('MQ==')+'jYmM='), key, iv);
    return Buffer.concat([cipher.update(data), cipher.final()]);
};

process.stdout.write('Enter The Flag: ');
process.stdin.on('data', (input) => {
    const inputStr = input.toString().trim();
    let result = 'Wrong';

    for (let key of keys) {
        if (Buffer.compare(encryptWithKey(inputStr, key), flag) === 0) {
            result = 'Correct!';
            break;
        }
    }

    console.log(result);
    process.exit();
});
