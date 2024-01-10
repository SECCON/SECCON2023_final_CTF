from flask import Flask, request, Response, render_template_string
import re

from util import *

app = Flask(__name__)
PASSWORD = read_file('password.txt')
PAGE_DIR = 'page'

def get_params(request):
    params = {}
    params.update(request.args)
    params.update(request.form)
    return params

@app.route('/', methods=['GET', 'POST'])
def index():
    page = get_params(request).get('page', 'index')

    path = os.path.join(PAGE_DIR, page) + '.txt'
    if os.path.isabs(path) or not within_directory(path, PAGE_DIR):
        return 'Invalid path'

    path = os.path.normpath(path)
    text = read_file(path)
    text = re.sub(r'SECCON\{.*?\}', '[[FLAG]]', text)

    if contains_word(path, PASSWORD):
        return 'Do not leak my password!'

    return Response(text, mimetype='text/plain')

@app.route('/premium', methods=['GET', 'POST'])
def premium():
    password = get_params(request).get('password')
    if password != PASSWORD:
        return 'Invalid password'

    page = get_params(request).get('page', 'index')
    path = os.path.abspath(os.path.join(PAGE_DIR, page) + '.txt')

    if contains_word(path, 'SECCON'):
        return 'Do not leak flag!'

    path = os.path.realpath(path)
    content = read_file(path)
    return render_template_string(read_file('premium.html'), path=path, content=content)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=80)
