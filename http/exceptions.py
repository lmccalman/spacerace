class APIException(Exception):
    """ Represents a generic API error."""

    def __init__(self, msg, status_code):
        self.msg = msg
        self.status_code = status_code

    def to_dict(self):
        return {'message': self.msg}


class APINotFoundException(APIException):
    """ Represents an API error where a resource was not found."""

    def __init__(self, msg):
        super().__init__(msg, 404)


class APIBadRequestException(APIException):
    """ Represents an API error where a request has invalid parameters."""

    def __init__(self, msg):
        super().__init__(msg, 400)
