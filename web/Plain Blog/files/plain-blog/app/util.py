import os

def resolve_dots(path):
    parts = path.split('/')
    results = []
    for part in parts:
        if part == '.':
            continue
        elif part == '..' and len(results) > 0 and results[-1] != '..':
            results.pop()
            continue
        results.append(part)
    return '/'.join(results)

def within_directory(path, directory):
    path = resolve_dots(path)
    return path.startswith(directory + '/')

def read_file(path):
    with open(os.path.abspath(path), 'r') as f:
        return f.read()

def contains_word(path, word):
    return os.path.exists(path) and word in read_file(path)
