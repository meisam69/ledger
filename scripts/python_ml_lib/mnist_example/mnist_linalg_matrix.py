# coding: utf-8

import os                                           # checking for already existing files
import gzip                                         # downloading/extracting mnist data
import random                                       # normal distirbution sampling
from tqdm import tqdm                               # visualising progress
import numpy as np                                  # loading data from buffer
import sys
import matplotlib.pyplot as plt
import math

# from fetch.math.linalg import MatrixDouble          # our matrices
from fetch.math.linalg import MatrixDouble
from fetch.ml import Layer, Variable, Session
from fetch.ml import CrossEntropyLoss
# from fetch.ml.layers import Layer          # our matrices

# TODO: Validation size must be same as batch size for the moment


DO_PLOTTING = 1

def plot_weights(layers):

    for layer in layers:
        plt.hist(layer.weights().data())
        plt.show()
        plt.close()



class MnistLearner():

    def __init__(self):

        self.data_url = 'http://yann.lecun.com/exdb/mnist/'

        self.x_tr_filename = 'train-images-idx3-ubyte.gz'
        self.y_tr_filename = 'train-labels-idx1-ubyte.gz'
        self.x_te_filename = 't10k-images-idx3-ubyte.gz'
        self.y_te_filename = 't10k-labels-idx1-ubyte.gz'


        self.training_size = 10000
        self.validation_size = 1000

        self.n_epochs = 30
        self.batch_size = 50
        self.alpha = 0.2

        self.mnist_input_size = 784         # pixels in 28 * 28 mnist images
        self.net = [10]                     # size of hidden layers
        self.mnist_output_size = 10         # 10 possible characters to recognise

        self.activation_fn = 'LeakyRelu'      # LeakyRelu might be good?
        self.layers = []
        self.sess = None
        self.initialise_network()

        self.X_batch = self.sess.Variable([self.batch_size, 784], "X_batch")
        self.Y_batch = self.sess.Variable([self.batch_size, 10], "Y_batch")




    def initialise_network(self):

        self.sess = Session()

        # definition of the network layers
        # for i in range(len(self.layers)):
        #     self.net.append(self.layers[i])
        self.net.append(self.mnist_output_size)

        self.layers.append(self.sess.Layer(self.mnist_input_size, self.net[0], self.activation_fn, "input_layer"))
        if len(self.net) > 2:
            for i in range(len(self.net) - 2):
                self.layers.append(self.sess.Layer(self.net[i], self.net[i + 1], self.activation_fn, "layer_" + str(i+1)))
        self.layers.append(self.sess.Layer(self.net[-1], self.mnist_output_size, self.activation_fn, "output_layer"))


        # copy preinitialised numpy weights over into our layers
        weights = np.load('weights.npy')
        for i in range(len(weights)):
            self.layers[i].weights().FromNumpy(weights[i])
            # for j in range(len(weights[i])):
            #     self.layers[i].weights().Set(i, j, weights[i][j])

        if DO_PLOTTING:
            plot_weights(self.layers)

        self.y_pred = self.layers[-1].Output()

        return

    def load_data(self, one_hot=True, reshape=None):
        x_tr = self.load_images(self.x_tr_filename, self.training_size, "X_train")
        y_tr = self.load_labels(self.y_tr_filename, self.training_size, "Y_train")
        x_te = self.load_images(self.x_te_filename, self.validation_size, "X_test")
        y_te = self.load_labels(self.y_te_filename, self.validation_size, "Y_test")

        if one_hot:
            y_tr_onehot = Session.Zeroes(self.sess, [y_tr.size(), self.mnist_output_size])
            y_te_onehot = Session.Zeroes(self.sess, [y_te.size(), self.mnist_output_size])

            for i in range(y_tr.size()):
                y_tr_onehot[i, int(y_tr[i])] = 1

            for i in range(y_te.size()):
                y_te_onehot[i, int(y_te[i])] = 1
            self.y_tr = y_tr_onehot
            self.y_te = y_te_onehot
        else:
            self.y_tr = y_tr
            self.y_te = y_te

        if reshape:
            x_tr, x_te = [x.reshape(*reshape) for x in (x_tr, x_te)]
        self.x_tr = x_tr
        self.x_te = x_te

    def load_images(self, filename, data_size, name):
        self.download(filename)
        with gzip.open(filename, 'rb') as f:
            data = np.frombuffer(f.read(), np.uint8, offset=16)
            data = data.reshape(-1, 28 * 28) / 256
            nd_data = self.sess.Variable([data_size, 784], name)
            nd_data.FromNumpy(data[:data_size, :])

        return nd_data

    def load_labels(self, filename, data_size, name):
        self.download(filename)
        with gzip.open(filename, 'rb') as f:
            data = np.frombuffer(f.read(), np.uint8, offset=8)
            data.reshape(np.shape(data)[0], -1)

            nd_data = self.sess.Variable([data_size, 1], name)
            nd_data.FromNumpy(data[:data_size].reshape(data_size, -1))
            # nd_data = Layer(data_size, 1, self.sess)
            # nd_data.data().FromNumpy(data[:data_size].reshape(data_size, -1))
        # return nd_data
        return nd_data

    def download(self, filename):
        if not os.path.exists(filename):
            from urllib.request import urlretrieve
            print("Downloading %s" % filename)
            urlretrieve(self.data_url + filename, filename)

        return

    # feed forward pass of a network
    # take X input and the network (defined as a list of weights)
    # no biases?
    def feed_forward(self, X):

        a = [X]
        activate = True
        for idx in range(len(self.layers)):
            if (idx == (len(self.layers) -1)):
                activate = False
            a.append(self.layers[idx].Forward(a[-1], activate))
        return a


    def calculate_loss(self, X, Y):

        loss = CrossEntropyLoss(X, Y, self.sess)
        return loss

    def do_one_pred(self):
        return self.sess.Predict(self.X_batch, self.layers[-1].Output())

    def predict(self):

        n_samples = self.y_te.shape()[0]

        preds = []
        if (self.batch_size < n_samples):
            for cur_rep in range(0, n_samples, self.batch_size):
                self.assign_batch(self.x_te, self.y_te, cur_rep)
                preds.append(self.do_one_pred())

        elif (self.batch_size > self.y_te.shape()[0]):
            print("not implemented prediction padding yet")
            raise NotImplementedError
        else:
            preds.append(self.do_one_pred())

        return preds

    def assign_batch(self, x, y, cur_rep):
        # assign X batch
        for k in range(self.batch_size):
            for l in range(28 * 28):
                self.X_batch[k, l] = x[cur_rep + k, l]

        # assign Y batch
        for k in range(self.batch_size):
            for l in range(10):
                self.Y_batch[k, l] = y[cur_rep + k, l]
        return

    def print_accuracy(self, cur_pred):

        max_pred = []
        for item in cur_pred:
            max_pred.extend(item.ArgMax(1))
        gt = self.y_te.data().ArgMax(1)

        sum_acc = 0
        for i in range(self.y_te.shape()[0]):
            sum_acc += (gt[i] == max_pred[i])
        sum_acc /= (self.y_te.data().size() / 10)

        print("\taccuracy: ", sum_acc)
        return


    def train(self):

        # TODO: Check if we need to re-call setinput for  each batch
        self.sess.SetInput(self.layers[0], self.X_batch)
        for i in range(len(self.layers) - 1):
            self.sess.SetInput(self.layers[i + 1], self.layers[i].Output())

        # epochs
        for i in range(self.n_epochs):
            print("epoch ", i, ": ")

            # training batches
            for j in tqdm(range(0, self.x_tr.shape()[0], self.batch_size)):

                # assign fresh data batch
                self.assign_batch(self.x_tr, self.y_tr, j)

                # loss calculation
                loss = self.calculate_loss(self.layers[-1].Output(), self.Y_batch)

                # back propagate
                self.sess.BackProp(self.X_batch, loss, self.alpha, 1)

                # print("output: " + str(self.layers[-1].Output()[0]))
                # print("gt: " + str(self.Y_batch[0]))
                #
                # sum_loss = 0
                # for i in range(loss.size()):
                #     sum_loss += loss.data()[i]
                # #     # print("\n")
                # #     # for j in range(1):
                # #     #     sys.stdout.write('{:0.13f}'.format(loss.data()[i]) + "\t")
                # print("sumloss: " + str(sum_loss))

            cur_pred = self.predict()

            self.print_accuracy(cur_pred)

        return


def run_mnist():

    mlearner = MnistLearner()

    # load the data
    mlearner.load_data(one_hot=True)

    # being training
    mlearner.train()



# import cProfile
# cProfile.run('run_mnist()')
run_mnist()