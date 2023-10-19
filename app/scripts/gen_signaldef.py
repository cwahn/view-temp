import os
import sys
import re

print('sys argv ', sys.argv[1], sys.argv[2])
sys.path.append(sys.argv[1])

def list_files_with_extensions(root_dir, extensions):
    matching_files = []
    for dirpath, dirnames, filenames in os.walk(root_dir):
        if 'build' in dirnames:
            dirnames.remove('build')

        for filename in filenames:
            if filename.endswith(tuple(extensions)):
                matching_files.append(os.path.join(dirpath, filename))
    return matching_files

root_directory = sys.argv[1]
extensions = ['.h', '.cpp', '.hpp']
statement = re.compile(r'SIGNAL_LOG\s*\(\s*([a-zA-Z0-9_]+)\s*,')#re.compile('\s*\bSIGNAL_LOG\s*\(\s*([a-zA-Z0-9_]+)\s*,')
signal_name = []

files = list_files_with_extensions(root_directory, extensions)

# print(files)

for file in files:
    with open(file, 'r') as file_s:
        result = statement.findall(file_s.read())
        if result:
            signal_name += result


if 'ID' in signal_name:
    signal_name.remove('ID')

if len(signal_name) == 0:
    signal_name = ["SIGNAL_EMPTY"]

print(signal_name)

with open(sys.argv[2] + '/signal_list.h', 'w') as tmp_h:
    s = '#ifndef __SIGNAL_LIST_H__\n' + \
        '#define __SIGNAL_LIST_H__\n\n' + \
        'static constexpr int8_t SIGNAL_NUM = ' + str(len(signal_name)) + ';\n' + \
        'enum SignalID : int8_t \n{\n\t' + ',\n\t'.join(signal_name) + '\n};\n' + \
        'enum SignalThreadID : int8_t \n{\n\t' + 's_no_name_thread = 0' + '\n};\n' + \
        'static constexpr const char *SIGNAL_NAME[SIGNAL_NUM] \n{\n\t' + ',\n\t'.join(['"' + s + '"' for s in signal_name]) + '\n};\n' + \
        'static SignalType SIGNAL_TYPE[SIGNAL_NUM]{t_yet};\n\n' + \
        'static SignalThreadID SIGNAL_THREAD[SIGNAL_NUM]{s_no_name_thread};\n\n' + \
        '#endif /* __SIGNAL_LIST_H__ */'
    tmp_h.write(s)
