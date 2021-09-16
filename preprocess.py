import os
import re
import json

import numpy as np
import pandas as pd
from tqdm import tqdm

FILTERS = "([~.,!?\"':;)(])"
PAD = "<PAD>"
SOS = "<SOS>"
EOS = "<EOS>"
UNK = "<UNK>"

PAD_INDEX = 0
SOS_INDEX = 1
EOS_INDEX = 2
UNK_INDEX = 3

MARKER = [PAD, SOS, EOS, UNK]
CHANGE_FILTER = re.compile(FILTERS)

MAX_SEQUENCE = 25


def load_data(path):
    # 판다스를 통해서 데이터를 불러온다.
    data_df = pd.read_csv(path, header=0)
    # 질문과 답변 열을 가져와 question과 answer에 넣는다.
    question, answer = list(data_df['Q']), list(data_df['A'])

    return question, answer

def data_tokenizer(data):
    # 토크나이징 해서 담을 배열 생성
    words = []
    for sentence in data:
        # FILTERS = "([~.,!?\"':;)(])"
        # 위 필터와 같은 값들을 정규화 표현식을
        # 통해서 모두 "" 으로 변환 해주는 부분이다.
        sentence = re.sub(CHANGE_FILTER, "", sentence)
        for word in sentence.split():
            words.append(word)
    # 토그나이징과 정규표현식을 통해 만들어진
    # 값들을 넘겨 준다.
    return [word for word in words if word]

def load_vocabulary(path, vocab_path):
    # 사전을 담을 배열 준비한다.
    vocabulary_list = []
    # 사전을 구성한 후 파일로 저장 진행한다.

    if not os.path.exists(vocab_path):
        # 이미 생성된 사전 파일이 없으므로, 사전을 생성해야 함
        if (os.path.exists(path)):
            # 사전을 만드는 대상인 데이터세트 불러오기
            data_df = pd.read_csv(path, encoding='utf-8')
            question, answer = list(data_df['Q']), list(data_df['A'])
            
            data = []
            # 질문과 답변을 extend을 통해서 1차원 배열로 만든다.
            data.extend(question)
            data.extend(answer)
            # 질문과 답변 문장으로부터 단어 리스트를 생성함
            words = data_tokenizer(data)
            # 단어 리스트 내 중복 단어 제거
            words = list(set(words))
            # 데이터 없는 내용중에 MARKER를 사전에 추가함
            # 아래는 MARKER 값이며 리스트의 첫번째 부터 순서대로 넣기 위해서 인덱스 0에 추가함
            # PAD = "<PADDING>"
            # SOS = "<START>"
            # EOS = "<EOS>"
            # UNK = "<UNKNWON>"
            words[:0] = MARKER
        # 사전을 리스트로 만들었으니 이 내용을 사전 파일을 만들어 넣는다.
        with open(vocab_path, 'w', encoding='utf-8') as vocabulary_file:
            for word in words:
                vocabulary_file.write(word + '\n')

    # 사전 파일이 존재하면 여기에서
    # 그 파일을 불러서 배열에 넣어 준다.
    with open(vocab_path, 'r', encoding='utf-8') as vocabulary_file:
        for line in vocabulary_file:
            vocabulary_list.append(line.strip())

    # 배열에 내용을 키와 값이 있는 딕셔너리 구조로 만든다.
    char2idx, idx2char = make_vocabulary(vocabulary_list)
    # 두가지 형태의 키와 값이 있는 형태를 리턴한다.
    # (예) 단어: 인덱스 , 인덱스: 단어)
    return char2idx, idx2char, len(char2idx)


def make_vocabulary(vocabulary_list):
    # 리스트를 키가 단어이고 값이 인덱스인
    # 딕셔너리를 만든다.
    char2idx = {char: idx for idx, char in enumerate(vocabulary_list)}
    # 리스트를 키가 인덱스이고 값이 단어인
    # 딕셔너리를 만든다.
    idx2char = {idx: char for idx, char in enumerate(vocabulary_list)}
    # 두개의 딕셔너리를 넘겨 준다.
    return char2idx, idx2char

def enc_processing(value, dictionary):

    sequences_input_index = [] # 입력으로 들어가는 모든 문장들에 대해 단어 인덱스 값들을 가지고 있는 배열

    # 데이터세트에서 문장 한 줄씩 불러와서
    for sequence in value:
        sequence = re.sub(CHANGE_FILTER, "", sequence) # 필터에 들어 있는 값들 "([~.,!?\"':;)(])"을  "" 으로 치환 한다.
        sequence_index = [] # 하나의 문장을 인코딩 할때 가지고 있기 위한 배열
        
        for word in sequence.split(): # 문장을 스페이스 단위로 자름
            if dictionary.get(word) is not None: # 잘려진 단어들이 사전에 존재 하는지 보고, 
                sequence_index.extend([dictionary[word]]) # 사전에 있다면, 단어의 인덱스 값을 가져와 sequence_index에 추가함.
            else: # 잘려진 단어가 사전에 존재 하지 않는 경우 이므로 UNK 토큰을 넣어 준다.
                sequence_index.extend([dictionary[UNK]])
                
        # 문장 제한 길이보다 길어질 경우, 토큰을 자름
        if len(sequence_index) > MAX_SEQUENCE:
            sequence_index = sequence_index[:MAX_SEQUENCE]
            
        # max_sequence_length보다 문장 길이가 작다면, 빈 부분에 PAD 토큰을 넣어줌.
        sequence_index += (MAX_SEQUENCE - len(sequence_index)) * [dictionary[PAD]]
        # 하나의 문장에 대해 인덱스화한 값을 sequences_input_index에 이어붙임
        sequences_input_index.append(sequence_index)
 
    return np.asarray(sequences_input_index)


def dec_output_processing(value, dictionary):

    sequences_output_index = [] # 모든 문장들에 대해 단어 인덱스 값들을 가지고 있는 배열
    
    for sequence in value:
        sequence = re.sub(CHANGE_FILTER, "", sequence)
        
        sequence_index = [] # 하나의 문장을 디코딩 할때 가지고 있기 위한 배열
        # 디코딩 입력의 처음에는 START 토큰이 와야함
        # 문장에서 스페이스 단위별로 단어를 가져와서 사전의 단어 인덱스 값을 넣어줌
        sequence_index = [dictionary[SOS]] + [dictionary[word] if word in dictionary else dictionary[UNK] for word in sequence.split()]
        
        # 문장 제한 길이보다 길어질 경우 뒤에 토큰을 자름
        if len(sequence_index) > MAX_SEQUENCE:
            sequence_index = sequence_index[:MAX_SEQUENCE]

        # max_sequence_length보다 문장 길이가 작다면, 빈 부분에 PAD 토큰을 넣어줌.
        sequence_index += (MAX_SEQUENCE - len(sequence_index)) * [dictionary[PAD]]
        
        # 하나의 문장에 대해 인덱스화한 값을 sequences_input_index에 이어붙임
        sequences_output_index.append(sequence_index)
        
    return np.asarray(sequences_output_index)

def dec_target_processing(value, dictionary):
    sequences_target_index = []

    for sequence in value:
        sequence = re.sub(CHANGE_FILTER, "", sequence)
        
        # 디코딩 출력의 마지막에 EOS 넣어 준다.
        sequence_index = [dictionary[word] if word in dictionary else dictionary[UNK] for word in sequence.split()]
        
        # 문장 제한 길이보다 길어질 경우 뒤에 토큰을 자르고, 마지막에 END 토큰을 넣어줌.
        if len(sequence_index) >= MAX_SEQUENCE:
            sequence_index = sequence_index[:MAX_SEQUENCE - 1] + [dictionary[EOS]]
        else:
            sequence_index += [dictionary[EOS]]
            
        # max_sequence_length보다 문장 길이가 작다면, 빈 부분에 PAD 토큰을 넣어줌.
        sequence_index += (MAX_SEQUENCE - len(sequence_index)) * [dictionary[PAD]]
        
        # 하나의 문장에 대해 인덱스화한 값을 sequences_input_index에 이어붙임
        sequences_target_index.append(sequence_index)
        
    return np.asarray(sequences_target_index)