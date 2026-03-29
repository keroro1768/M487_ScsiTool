const {execSync} = require('child_process');
try {
    const r = execSync('"C:\\Users\\rinry\\AppData\\Local\\Programs\\Python\\Python314\\python.exe" "C:\\Users\\rinry\\Tool\\usb_list.py"', {encoding:'utf8', timeout:20000});
    console.log(r);
} catch(e) {
    console.log('FAIL:', e.stderr || e.message || e);
}
