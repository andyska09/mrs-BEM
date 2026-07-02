import argparse

from config.settings import Settings
from learner import Learner


def main():
    parser = argparse.ArgumentParser(description='Test simulator network.')
    parser.add_argument('--settings_file', help='Path to settings yaml', required=True)

    args = parser.parse_args()
    settings_filepath = args.settings_file

    settings = Settings(settings_filepath, generate_log=False)

    learner = Learner(settings)
    learner.test()


if __name__ == "__main__":
    main()
