from .api import api
from .exceptions import APIException
from .response import returns_json
import json


@api.errorhandler(APIException)
@returns_json
def handle_generic_api_error(error):
    """ For API errors, we just convert the exception into JSON."""
    return json.dumps(error.to_dict()), error.status_code
