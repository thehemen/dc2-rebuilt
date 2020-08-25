import os
import re
import json
import tqdm
import glob
import time
import subprocess
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
    dataset_path = '../../.dc2_short/10k_all/'

    lang_conf_threshold = {'en': 90.0, 'ru': 50.0}
    all_filenames = sorted(glob.glob(dataset_path + '*.html'))
    lang_dict = {}

    for filename in all_filenames:
        lang_dict[get_short_name(filename)] = {'predicted': 'other', 'real': 'other'}

    os.chdir('../bin')
    begin_time = time.time()
    result = subprocess.run(['./tgnews', 'languages', dataset_path],
        stdout=subprocess.PIPE)
    diff_time = time.time() - begin_time
    json_out = json.loads(result.stdout)
    print('The program \'./tgnews\' has run for {:.1f} seconds.'.format(diff_time))

    for filename in json_out[0]['articles']:
        lang_dict[filename]['predicted'] = 'en'

    for filename in json_out[1]['articles']:
        lang_dict[filename]['predicted'] = 'ru'

    for i in tqdm.tqdm(range(len(all_filenames))):
        filename = all_filenames[i]
        header, plaintext = parsehtml(readfile(filename))
        text = removetabs(removechars(plaintext))

        if len(text) == 0:
            continue

        lang_best = Detector(text, quiet=True).languages[0]
        lang_best_name = str(lang_best.__dict__['locale'])
        lang_best_conf = lang_best.__dict__['confidence']

        for lang_name, lang_threshold in lang_conf_threshold.items():
            if lang_name == lang_best_name:
                if lang_best_conf > lang_threshold:
                    lang_dict[get_short_name(filename)]['real'] = lang_name
                break

    TP = {'en': 0, 'ru': 0}  # True Positive
    TN = {'en': 0, 'ru': 0}  # True Negative
    FP = {'en': 0, 'ru': 0}  # False Positive
    FN = {'en': 0, 'ru': 0}  # False Negative

    for filename, pred_real_dict in lang_dict.items():
        predicted_lang = pred_real_dict['predicted']
        real_lang = pred_real_dict['real']

        for language in ['en', 'ru']:
            if real_lang == language and predicted_lang == language:
                TP[language] += 1
            elif real_lang != language and predicted_lang != language:
                TN[language] += 1
            elif real_lang != language and predicted_lang == language:
                FP[language] += 1
            elif real_lang == language and predicted_lang != language:
                FN[language] += 1

    accuracy = {'en': 0, 'ru': 0}
    precision = {'en': 0, 'ru': 0}
    recall = {'en': 0, 'ru': 0}

    for l in ['en', 'ru']:
        accuracy[l] = (TP[l] + TN[l]) / (TP[l] + TN[l] + FP[l] + FN[l])
        precision[l] = TP[l] / (TP[l] + FP[l])
        recall[l] = TP[l] / (TP[l] + FN[l])

    for l in ['en', 'ru']:
        print('\n{}:'.format(l))
        print('\tAccuracy: {:.6f}'.format(accuracy[l]))
        print('\tPrecision: {:.6f}'.format(precision[l]))
        print('\tRecall: {:.6f}'.format(recall[l]))
