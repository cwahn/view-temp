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

statement_thread = re.compile(r'SIGNAL_INIT_LOG_\s*\(\s*([a-zA-Z0-9_]+)\s*\)')#re.compile('\s*\bSIGNAL_LOG\s*\(\s*([a-zA-Z0-9_]+)\s*,')
thread_name = []

files = list_files_with_extensions(root_directory, extensions)

# print(files)

for file in files:
    with open(file, 'r') as file_s:
        src = file_s.read()
        result = statement.findall(src)
        if result:
            signal_name += result

        result_t = statement_thread.findall(src)
        if result_t:
            thread_name += result_t



if 'SNID' in signal_name:
    signal_name.remove('SNID')

if 'TID' in thread_name:
    thread_name.remove('TID')


if len(signal_name) == 0:
    signal_name = ["SIGNAL_EMPTY"]

if len(thread_name) == 0:
    thread_name += ["THREAD_EMPTY"]


# thread_name has defaultly two element
thread_name += ["err_not_inited_thread"]


print(signal_name)
print(thread_name)


with open(sys.argv[2] + '/signal_list.h', 'w') as tmp_h:
    s = '#ifndef __SIGNAL_LIST_H__\n' + \
        '#define __SIGNAL_LIST_H__\n\n' + \
        '// Python preprocessor used to remove thread dependency while initialization step.\n\n' + \
        'static constexpr int8_t SIGNAL_NAME_NUM = ' + str(len(signal_name)) + ';\n' + \
        'static constexpr int8_t SIGNAL_THREAD_NUM = ' + str(len(thread_name) - 1) + ';\n' + \
        'enum SignalNameID : int8_t \n{\n\t' + ',\n\t'.join(signal_name) + '\n};\n' + \
        'enum SignalThreadID : int8_t \n{\n\t' + ',\n\t'.join(thread_name) + '\n};\n' + \
        'static constexpr const char *SIGNAL_NAME[SIGNAL_NAME_NUM] \n{\n\t' + ',\n\t'.join(['"' + s + '"' for s in signal_name]) + '\n};\n' + \
        'static constexpr const char *SIGNAL_THREAD_NAME[SIGNAL_THREAD_NUM + 1] \n{\n\t' + ',\n\t'.join(['"' + s + '"' for s in thread_name]) + '\n};\n' + \
        'typedef int8_t SignalID;\n' + \
        '#endif /* __SIGNAL_LIST_H__ */' 
    tmp_h.write(s)
