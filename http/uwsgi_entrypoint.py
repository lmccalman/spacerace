#!/usr/bin/python3
import flask as fl
from . import factory

app = factory.create_app(__name__)
