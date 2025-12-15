from setuptools import find_packages, setup

package_name = 'serial_teleop'

setup(
    name=package_name,
    version='0.0.1',
    packages=find_packages(),
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ubuntu',
    maintainer_email='ubuntu@todo.todo',
    description='Keyboard teleop to serial port for Bob the robot',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            'serial_teleop = serial_teleop.serial_teleop:main',
            'serial_teleop_extended = serial_teleop.serial_teleop_extended:main',
        ],
    },
)
