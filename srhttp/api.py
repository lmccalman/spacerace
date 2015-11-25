import flask as fl
from flask.ext.cors import CORS

api = fl.Blueprint('statbadger_api', __name__)
CORS(api)
