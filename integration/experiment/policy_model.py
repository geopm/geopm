#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# TODO docstring
# TODO appropriate imports

class PolicyModel(object):
    """
    Abstract class for predicting a statistic based on one or more
    independent variables."""
    def __init__(self):
        "Default constructor."
        raise NotImplementedError

    def train(self, df, key, index_list):
        """
        Train this model to fit the data of column key contained in the
        dataframe (df), which is indexed by the independent variable(s). This
        modifies the instance it is called on, preparing it for calls of the
        methods test and batch_test."""
        raise NotImplementedError

    def evaluate(self, var_list):
        """
        Evaluate a trained model at a single setting of the independent
        variables. Returns the model's prediction."""
        raise NotImplementedError

    def batch_evaluate(self, var_list_list):
        """
        Evaluate a trained model on a list-like container of independent
        variable settings (var_list_list). Returns a list of values."""
        return [self.evaluate(var_list) for var_list in var_list_list]

    def serialize(self):
        "Convert model to JSON."
        raise NotImplementedError

    def __str__(self):
        return "<%s>" % "PolicyModel"

    def repr(self):
        return self.__str__()


class PolynomialFitPolicyModel(PolicyModel):
    """
    Implementation of a polynomial model of fixed degree that can be a
    trained to predict a statistic from one or more independent variables."""
    def __init__(self, degree):
        """
        Default constructor; degree (a positive integer) is the degree
        of the polynomial being fit to."""
        self._degree = degree
        self._model = None

    def train(self, df, key, index_list):
        AA = df.reset_index()[index_list]
        
        cols = ['1'] + index_list
 
        AA['1'] = 1
        for ii in range(2, self._degree+1):
            for idx in index_list:
                col = idx + '**' + str(ii)
                cols.append(col)
                AA[col] = AA[idx] ** ii

        AA = AA[cols]

        new_series, resid, rank, s = np.linalg.lstsq(AA,
                                                     df[key].values,
                                                     rcond = None)
        resid = sum((df[key].values - np.dot(AA, new_series))**2)
        self._model = new_series
        # numpy's least squares method doesn't consistently return residuals
        # so we compute them manually
        self._resid = (resid / len(df[key].values))**0.5

    def evaluate(self, var_list):
        xx = [1]
        for ii in range(1, self._degree+1):
            for ff in var_list:
                xx.append(ff ** ii)

        return sum(self._model * np.array(xx))

    def residual(self):
        return self._resid


class CubicPolicyModel(PolynomialFitPolicyModel):
    """
    Implementation of a cubic model that can be a trained to predict a
    statistic from a set of variables."""
    def __init__(self):
        "Simple constructor."
        super(CubicPolicyModel, self).__init__(degree=3)


class InversePolicyModel(PolicyModel):
    """
    Implementation of a plicy model that predicts a statistic that is
    linear in the inverses of independent variables."""
    def __init__(self):
        self._model = None

    def train(self, df, key, index_list):
        AA = df.reset_index()[index_list]
        
        cols = ['1']
 
        AA['1'] = 1
        for idx in index_list:
            col = idx + 'inv'
            cols.append(col)
            AA[col] = 1./AA[idx]

        AA = AA[cols]

        new_series, resid, rank, s = np.linalg.lstsq(AA,
                                                     df[key].values,
                                                     rcond = None)
        resid = sum((df[key].values - np.dot(AA, new_series))**2)
        self._model = new_series
        # numpy's least squares method doesn't consistently return residuals
        # so we compute them manually
        self._resid = (resid / len(df[key].values))**0.5

    def evaluate(self, var_list):
        xx = [1.]
        for ff in var_list:
            xx.append(1./ff)
        return sum(self._model * np.array(xx))

    def residual(self):
        return self._resid

class CrossValidationModel(PolicyModel):
    """
    Implementation of a Cross Validation model. This creates an ensemble of
    sub-models, gives each one a fraction of the data, and deduces confidence
    values from the distribution of fit values."""
    def __init__(self, base_gen, frac=0.8, size=100):
        """
        Simple constructor; base_gen is a callable that returns a
        PolicyModel instance (or a derived class), frac is the proportion
        of training data to give to each instance during training (default 0.8),
        and size is the number of sub-models to instantiate for the ensemble
        (default 100)."""

        self._model = [base_gen() for _ in range(size)]
        self._frac = frac

    def train(self, df, key, index_list):
        for mm in self._model:
            mm.train(df.sample(frac=self._frac), key)

    def evaluate(self, var_list, full=False):
        """
        Evaluate a trained model on a single setting of the independent
        variables. Returns the model's prediction for this frequency. If the
        parameter full is True, also return the standard deviation and residuals
        (default False)."""

        vals = [mm.evaluate(freq_list) for mm in self._model]
        mean = sum(vals)/len(vals)
        dev = (sum([val**2 for val in vals])/len(vals) - mean**2)**0.5
        res = (sum([mm.residual() for mm in self._model])/len(self._model))
        if full:
            return mean, [dev, res]
        return mean


def extract_columns(df, region_filter = None, keys = {'FREQ_DEFAULT': 'freq', 'FREQ_UNCORE': 'uncore'}):
    """
    Extract the columns of interest from the full report collection
    dataframe. This returns a dataframe indexed by the provided
    keys and columns 'runtime' and 'energy'. region_filter (if provided)
    is a container that specifies which regions to include (by default,
    include all of them)."""
    df_filtered = df
    if region_filter and region_filter != 'Epoch':
        df_filtered = df[df['region'].isin(region_filter.split(','))]

    # these are the only columns we need
    df_cols = df_filtered[list(keys.keys()) +
                          ['host', 'Profile',
                           'runtime (sec)',
                           'package-energy (joules)']]

    rename_dict = {'runtime (sec)': 'runtime', 'package-energy (joules)': 'energy'}
    rename_dict.update(keys)
    df_cols = df_cols.rename(rename_dict, axis = 1)

    df_cols = df_cols.groupby(list(rename_dict.values()) + ['host', 'Profile']).sum().reset_index()
    df_cols = df_cols[list(rename_dict.values())]
    df_cols = df_cols.set_index(list(keys.values()))

    return df_cols


def dump_stats_summary(df, fname):
    """
    Write mean runtime and energy and the standard deviation of
    runtime and energy for each index in CSV format to the
    file fname."""
    means = df.groupby(level=0).mean().rename(lambda x: x+"_mean", axis=1)
    stds = df.groupby(level=0).std().rename(lambda x: x+"_stddev", axis=1)
    pandas.concat([means, stds], axis=1).to_csv(fname)


def policy_min_energy(index_range, enmodel, rtmodel = None, sticker = None,
                      max_degradation = None):
    """
    Find the setting over the range index_range (list-like) that
    has the minimum predicted energy usage (according to the energy model
    enmodel, an instance of PolicyModel), subject to the constraint
    that its runtime does not exceed the runtime at the reference (sticker)
    by more than a factor of (1 + max_degradation), if max_degradation is
    specified. Returns a dictionary with keys index, runtime, and energy,
    and values the index and the predicted runtime and energy
    at that limit, respectively."""

    if sticker is None:
        sticker = max(index_range)

    en_predictions = enmodel.batch_evaluate(index_range)
    if max_degradation is None:
        # we don't need the runtime model in this case
        best_energy, best_index = min(zip(en_predictions, index_range))
        if rtmodel:
            best_runtime = rtmodel.evaluate(best_index)
        else:
            best_runtime = None
        return {'index': best_index,
                'runtime': best_runtime,
                'energy': best_energy}
    else:
        rt_predictions = rtmodel.batch_evaluate(index_range)
        rt_at_sticker = rtmodel.evaluate(sticker)
        constrained_values = [(energy, runtime, index)
                              for index, runtime, energy
                              in zip(index_range, rt_predictions, en_predictions)
                              if runtime <= (1 + max_degradation) * rt_at_sticker]
        best_energy, best_runtime, best_index = min(constrained_values)
        return {'index': best_index,
                'runtime': best_runtime,
                'energy': best_energy}


def rootssq(ll):
    "Return the root of the sum of the squares of the list of values."
    try:
        return sum([ss**2  for ss in ll])**0.5
    except TypeError:
        pass
    return ll

def normal_comparison(dist1, dist2):
    """Returns the probability that a sample drawn from the normal distribution
    described by dist1 (a tuple of mu and sigma) will be larger than a sample
    drawn from a normal distribution drawn from the normal distribution
    described by dist2."""
    mu1, sigma1 = dist1
    mu2, sigma2 = dist2
    sigma1 = rootssq(sigma1)
    sigma2 = rootssq(sigma2)
    return 0.5 * (1 + math.erf((mu1 - mu2) / (2 * (sigma1 ** 2 + sigma2 ** 2)**0.5)))


def policy_confident_energy(index_range, enmodel, rtmodel = None,
                            sticker = None, max_degradation = None,
                            confidence = 0.9):
    """
    Find the setting over the range index_range (list-like) that
    has the minimum predicted energy usage (according to the energy model
    enmodel, an instance of FrequencyModel), subject to the constraint
    that its runtime does not exceed the runtime at the reference (sticker)
    by more than a factor of (1 + max_degradation), if max_degradation is
    specified, with at least a confidence (default 0.9) probability. Returns a
    dictionary with keys index, runtime, and energy, and values the optimal
    index, the predicted runtime and energy at that limit,
    respectively, and the standard deviations of these values."""
    if sticker is None:
        sticker = max(index_range)

    en_predictions = {
        index: enmodel.evaluate(index, True)
        for index in index_range}

    if max_degradation is None:
        # we don't need the runtime model in this case
        best_energy, best_index = min(zip(en_predictions.values(), en_predictions.keys()))
        if rtmodel:
            best_runtime = rtmodel.evaluate(best_index, True)
        else:
            best_runtime = None
        return {'index': best_index,
                'runtime': best_runtime[0],
                'runtimedev': rootssq(best_runtime[1]),
                'energy': best_energy[0],
                'energydev': rootssq(best_energy[1])}
    else:
        rt_predictions = {
                index: rtmodel.evaluate(index, True)
                for index in index_range}
        rt_at_sticker, [dev_sticker, res_sticker] = rtmodel.evaluate(sticker, True)
        degraded_rt = rt_at_sticker * (1 + max_degradation),\
                [dev_sticker * (1 + max_degradation), res_sticker * (1 + max_degradation)]
        constrained_values = [(energy, runtime, index)
                              for index, runtime, energy
                              in zip(index_range, rt_predictions, en_predictions)
                              if normal_comparison(degraded_rt, runtime) > confidence]
        if len(constrained_values) == 0:
            # this means that we don't ever have enough confidence to guarantee this
            return None
        best_energy, best_runtime, best_index = min(constrained_values)
        return {'index': best_index,
                'runtime': best_runtime[0],
                'runtimedev': rootssq(best_runtime[1]),
                'energy': best_energy[0],
                'energydev': rootssq(best_energy[1])}
