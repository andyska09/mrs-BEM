import argparse

from config.settings import Settings
from learner import Learner


def main():
    parser = argparse.ArgumentParser(description='Train simulator network.')
    parser.add_argument('--settings_file', help='Path to settings yaml', required=True)

    args = parser.parse_args()
    settings_filepath = args.settings_file

    settings = Settings(settings_filepath)

    learner = Learner(settings)
    learner.train()


if __name__ == "__main__":
    main()
