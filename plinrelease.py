#!/usr/bin/env /usr/bin/python3

import argparse
import os
import shutil
import subprocess
import multiprocessing

NAME = 'SimpleIDE'
SIMPLE_IDE_SOURCE_ROOT = os.path.dirname(os.path.realpath(__file__))

PROP_LOADER_PATH = os.path.join(SIMPLE_IDE_SOURCE_ROOT, 'PropLoader')
CTAGS_PATH = os.path.join(SIMPLE_IDE_SOURCE_ROOT, 'ctags-5.8')
SETUP_SCRIPT = os.path.join(SIMPLE_IDE_SOURCE_ROOT, 'release', 'linux', 'setup.sh')
SETUP_RUN_SCRIPT = os.path.join(SIMPLE_IDE_SOURCE_ROOT, 'release', 'linux', 'simpleide.sh')
SPIN_LIBRARIES = os.path.join(SIMPLE_IDE_SOURCE_ROOT, 'spin')
PROPSIDE_PATH = os.path.join(SIMPLE_IDE_SOURCE_ROOT, 'propside')
WX_LOADER = os.path.join(SIMPLE_IDE_SOURCE_ROOT, 'PropLoader')


class QmakeNotFoundException(Exception):
    def __init__(self):
        super().__init__('Qt must be installed. Please install Qt5.4 from here:' + os.linesep +
                         'download.qt.io./official_releases/qt/5.4/5.4.2')


class Qt5NotAvailableException(Exception):
    def __init__(self):
        super().__init__('The qmake program must be Qt5.4 or higher vintage.' + os.linesep +
                         'Please adjust PATH to include a Qt5 build if necessary.' + os.linesep +
                         'Please install Qt5.4 from here if you don\'t have it already:' + os.linesep +
                         'download.qt.io./official_releases/qt/5.4/5.4.2')


def run():
    args = parse_args()

    binary_root = args.build

    simple_ide_binary = compile_simple_ide(binary_root)
    package_binary_path = create_package_path(binary_root)
    install_static_files(package_binary_path)

    install_shared_libs(package_binary_path, simple_ide_binary)


def compile_simple_ide(binary_root):
    qmake_args = get_qmake_invocation()
    propside_binary_root = os.path.join(binary_root, 'propside')

    os.makedirs(propside_binary_root, exist_ok=True)
    invoke(qmake_args + [PROPSIDE_PATH], propside_binary_root)
    # invoke(['make', 'clean'], propside_binary_root) FIXME (just need to uncomment this)
    jobs_argument = '-j%d' % (multiprocessing.cpu_count() + 1)
    invoke(['make', jobs_argument], propside_binary_root)

    return os.path.join(propside_binary_root, 'SimpleIDE')


def create_package_path(binary_root):
    with open(os.path.join(PROPSIDE_PATH, 'propside.pro'), 'r') as propside_project_file:
        lines = propside_project_file.readlines()
    version_lines = [line.strip() for line in lines if 'VERSION=' in line]
    version_numbers = [line.split('VERSION=')[1] for line in version_lines]
    full_version_number = '.'.join(version_numbers)
    full_version = '%s-%s' % (NAME, full_version_number)
    print('Creating Version ' + full_version)
    package_binary_path = os.path.join(binary_root, full_version)
    return package_binary_path


def install_static_files(package_binary_path):
    shutil.rmtree(package_binary_path)
    os.makedirs(package_binary_path, exist_ok=True)
    release_template_path = os.path.join(SIMPLE_IDE_SOURCE_ROOT, 'release', 'template')
    for source_root, dirs, files in os.walk(release_template_path):
        relative_root = source_root[len(release_template_path):]
        if relative_root.startswith('/'):
            relative_root = relative_root[1:]

        destination_root = os.path.join(package_binary_path, relative_root)
        os.makedirs(destination_root, exist_ok=True)

        for f in files:
            source = os.path.join(source_root, f)
            destination = os.path.join(destination_root, f)
            shutil.copy2(source, destination)
    setup_script = shutil.copy2(SETUP_SCRIPT, package_binary_path)
    setup_run_script = shutil.copy2(SETUP_RUN_SCRIPT, os.path.join(package_binary_path, 'bin'))
    os.chmod(setup_script, 0o744)
    os.chmod(setup_run_script, 0o755)
    shutil.copytree(os.path.join(PROPSIDE_PATH, 'translations'), os.path.join(package_binary_path, 'translations'))


def install_shared_libs(package_binary_path, simpleide_binary):
    all_libs = subprocess.check_output(['ldd', simpleide_binary]).decode().split(os.linesep)
    qt_libs = [lib.strip().split()[2] for lib in all_libs if 'libQt' in lib or 'libaudio' in lib]
    for lib in qt_libs:
        shutil.copy(lib, os.path.join(package_binary_path, 'bin'))


def invoke(args, cwd):
    print(' '.join(args))
    subprocess.call(args, cwd=cwd)


def get_qmake_invocation():
    qmake_path = which('qmake')
    if not qmake_path:
        raise QmakeNotFoundException

    real_qmake_path = os.path.realpath(qmake_path)
    if 'qtchooser' == os.path.basename(real_qmake_path):
        raw_output = subprocess.check_output([real_qmake_path, '-list-versions'])
        qt_versions = raw_output.decode().split()
        if '5' in qt_versions or 'qt5' in qt_versions:
            return [qmake_path, '-qt=5']
        else:
            raise Qt5NotAvailableException()
    else:
        raw_output = subprocess.check_output([qmake_path, '-version'])
        string_output = raw_output.decode()
        if 'qt5' in string_output.lower():
            return [qmake_path]
        else:
            raise Qt5NotAvailableException()


def find(name, default_paths, search_path):
    """
    Find a file
    :param name: Name of the file to find 
    :type name str
    :param default_paths: Default paths where the file might be located
    :type default_paths list
    :param search_path: Root directory where a search should be performed if the file is not in the default location
    :type search_path str
    :return: Returns the path of the requested file
    :raises IOError If the file cannot be found
    """
    for default in default_paths:
        default = os.path.join(default, name)
        if os.path.exists(default):
            return os.path.realpath(default)
    for root, dirs, files in os.walk(search_path):
        if name in files:
            return os.path.realpath(os.path.join(root, name))


def is_exe(fpath):
    return os.path.isfile(fpath) and os.access(fpath, os.X_OK)


def which(program):
    file_path, filename = os.path.split(program)
    if file_path:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('-p', '--package', default=NAME + '.zip',
                        help='Name (and path) of the compressed package that will be generated')
    parser.add_argument('-g', '--propgcc', default='/opt/parallax',
                        help='PropGCC installation path on the local machine (will not affect installation path of '
                             'target machine)')
    parser.add_argument('-b', '--build', default=os.path.join(SIMPLE_IDE_SOURCE_ROOT, 'build'),
                        help='Directory where the build for SimpleIDE can take place')

    return parser.parse_args()


if '__main__' == __name__:
    run()
