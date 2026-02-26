from setuptools import setup, find_packages

setup(
    name="gym_sts_lightspeed",
    version="0.0.1",
    packages=find_packages(),
    install_requires=[
        "gymnasium",
        "numpy",
    ],
)
