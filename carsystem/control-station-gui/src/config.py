import yaml
import os

class Config:
    def __init__(self, yaml_file, logger):
        self.yaml_file = yaml_file
        self.logger = logger
        self.data = None
        self.loaded = False

    def load(self):
        if not os.path.isfile(self.yaml_file):
            self.logger.error("Yaml file does not exist.")
            return False

        with open(self.yaml_file, 'r') as f:
            self.data = yaml.load(f)
            self.loaded = True
        return True

    def getProperty(self, config_tag):
        if not self.loaded:
            return None

        try:
            return self.data[config_tag]
        except Exception as e:
            self.logger.error(f"Can't find property {config_tag}. Error {e}")
            return None

    def getDictionary(self):
        return self.data
