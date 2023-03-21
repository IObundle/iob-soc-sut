# This script is called during setup.
# You can use 'setup_module' to access the contents of the iob_soc_setup.py python module
import os
import shutil

# Find out correct test.expected filename
test_file_name='test'
#Check if setup with INIT_MEM (check if macro exists)
macro = next((i for i in setup_module.confs if i['name']=='INIT_MEM'), False)
if macro and macro['val']:
    test_file_name+='_initmem'
#Check if setup with USE_EXTMEM (check if macro exists)
macro = next((i for i in setup_module.confs if i['name']=='USE_EXTMEM'), False)
if macro and macro['val']:
    test_file_name+='_extmem'
test_file_name+='.expected'

dirpath=os.path.join(setup_module.build_dir, "hardware/simulation/src")

# Copy correct test.expected file to build dir
shutil.copyfile(os.path.join(dirpath, test_file_name),os.path.join(setup_module.build_dir, "hardware/simulation/test.expected"))

# Delete all test*.expected files from build_dir
for file in os.listdir(dirpath):
    if file.startswith("test") and file.endswith(".expected"):
        os.remove(os.path.join(dirpath,file))

