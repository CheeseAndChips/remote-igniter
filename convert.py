import subprocess

formstr = '#ifndef WEB_H\n#define WEB_H\nconst char web[] = {{{}}};\n#endif'
outp = subprocess.run(['minify', './src/index.html'], capture_output=True).stdout
# outp = subprocess.run(['cat', './src/index.html'], capture_output=True).stdout
final_output = formstr.format(str(['0x' + bytes([i]).hex() for i in outp]).replace('\'', '')[1:-1])

with open('./include/web_include.h', 'w') as f:
    f.write(final_output)