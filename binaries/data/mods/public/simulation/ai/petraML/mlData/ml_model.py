# -*- coding: utf-8 -*-
import numpy as np

import pandas as pd
import pickle

dataset = pd.read_csv('D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\newFull.csv')
x = dataset.iloc[:, :-1].values
y = dataset.iloc[:, -1].values
from sklearn.model_selection import train_test_split
from sklearn.model_selection import KFold
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import confusion_matrix
from sklearn.linear_model import LogisticRegression


kf = KFold(n_splits=3,random_state = 0)
classifier = LogisticRegression(random_state = 0)






for train_index, test_index in kf.split(x):
    
    print('%' * 50)
    X_train = x[train_index]
    X_train_symmetry = np.concatenate([X_train[:, 416:], X_train[:, :416]], axis = 1)
    X_train = np.concatenate([X_train, X_train_symmetry], axis = 0)
    y_train = y[train_index]
    y_train_symmetry = 1 - y_train
    y_train = np.concatenate([y_train, y_train_symmetry])
    X_test = x[test_index]
    y_test = y[test_index]
    classifier.fit(X_train,y_train)
    y_train_pred = classifier.predict(X_train)
    y_test_pred = classifier.predict(X_test)

    cm_train = confusion_matrix(y_train, y_train_pred)
    cm_train = cm_train * (100 / cm_train.sum())
    cm_test = confusion_matrix(y_test, y_test_pred)
    cm_test = cm_test * (100 / cm_test.sum())
    
    print('cm train:')
    print(cm_train)
    print('cm test')
    print(cm_test)
    
classifier.fit(x, y)

with open('D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\classifier.pkl', 'wb') as f:
    pickle.dump(classifier, f)
    
PROBA_RESCALING_N_SAMPLES = 100
PROBA_RESCALING_WINDOW_RADIUS = 0.05
y_proba = classifier.predict_proba(x)
y_proba = y_proba[:, 1]
y_proba_rescaling = {proba: y[np.abs(y_proba - proba) <= PROBA_RESCALING_WINDOW_RADIUS].mean() for proba in np.linspace(0, 1, PROBA_RESCALING_N_SAMPLES)}

with open('D:\\Users\\micha\\Desktop\\ram-and-michael\\binaries\\data\\mods\\public\\simulation\\ai\\petraML\\mlData\\proba_rescale.pkl', 'wb') as f:
    pickle.dump(y_proba_rescaling, f)
    
    















