import os
import re
import glob
import tqdm
import shutil
import logging
from bs4 import BeautifulSoup
from polyglot.detect import Detector

def get_short_name(complete_name):
    return complete_name.split('/')[-1]

def readfile(filename):
    with open(filename, 'r') as f:
        return f.read()

def parsehtml(rawtext):
    soup = BeautifulSoup(rawtext, features='html.parser')
    header = soup('h1')[0].string
    plaintext = ''
    for p in soup('p'):
        plaintext += p.get_text() + '\n'
    return header, plaintext

def removetabs(tabtext):
    return re.sub('\s+', ' ', tabtext)

def removechars(chartext):
    return ''.join(e for e in chartext if e.isalnum() or e.isspace())

if __name__ == '__main__':
    logging.getLogger('polyglot').setLevel(logging.CRITICAL)
    complete_path = '../../.dc2_complete/'
    short_path = '../../.dc2_short/'

    filenames = sorted(glob.glob(complete_path + '**/*.html', recursive=True))
    lang_conf_threshold = {'en': 90.0, 'ru': 50.0}

    if os.path.exists(short_path):
        shutil.rmtree(short_path)
    os.mkdir(short_path)

    for pathformat, limit in [('{}1k_{}', 1000), ('{}10k_{}', 10000)]:
        all_pathname = pathformat.format(short_path, 'all')
        os.mkdir(all_pathname)

        for i in tqdm.tqdm(range(limit)):
            filename = filenames[i]
            shutil.copyfile(filename, '{}/{}'.format(all_pathname, get_short_name(filename)))

        lang_paths = {}
        for language in ['en', 'ru']:
            lang_pathname = pathformat.format(short_path, language)
            os.mkdir(lang_pathname)
            lang_paths[language] = lang_pathname

        lang_copied_num = {'en': 0, 'ru': 0}
        i = 0
        with tqdm.tqdm(total=limit * 2) as tqdm_bar:
            for i in range(len(filenames)):
                filename = filenames[i]
                header, plaintext = parsehtml(readfile(filename))
                text = removetabs(removechars(plaintext))

                if len(text) == 0:
                    continue

                lang_best = Detector(text, quiet=True).languages[0]
                lang_best_name = str(lang_best.__dict__['locale'])
                lang_best_conf = lang_best.__dict__['confidence']

                if lang_best_name in ['en', 'ru']:
                    if lang_best_conf >= lang_conf_threshold[lang_best_name]:
                        if lang_copied_num[lang_best_name] < limit:
                            lang_copied_num[lang_best_name] += 1
                            tqdm_bar.update(1)
                            shutil.copyfile(filename, '{}/{}'.format(lang_paths[lang_best_name], get_short_name(filename)))

                if lang_copied_num['en'] == limit and lang_copied_num['ru'] == limit:
                    break