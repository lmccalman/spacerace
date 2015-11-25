#!/usr/bin/python3
import flask as fl
from srhttp import factory

app = factory.create_app(__name__)
