class Normalization:
    def __init__(self, normalization_coeffs):
        self.normalization_coeffs = normalization_coeffs

    def get_coeffs(self):
        return self.normalization_coeffs

    def normalize_inputs(self, features):
        '''

        :param features:
        :return:
        '''
        features = (features - self.normalization_coeffs[0]) / self.normalization_coeffs[1]
        return features

    def normalize_labels(self, labels):
        '''

        :param labels:
        :return:
        '''
        labels = (labels - self.normalization_coeffs[2]) / self.normalization_coeffs[3]
        return labels

    def unnormalize_inputs(self, features):
        '''

        :param features:
        :return:
        '''
        features = features * self.normalization_coeffs[1] + self.normalization_coeffs[0]
        return features

    def unnormalize_labels(self, labels):
        '''

        :param labels:
        :return:
        '''
        labels = labels * self.normalization_coeffs[3] + self.normalization_coeffs[2]
        return labels
