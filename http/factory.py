from .api import api
import flask as fl

# Register routes and handlers
from . import routes, error_handlers


def create_app(name):
    """ Factory method for creating an app.

    Returns:
        app: A Flask app configured with a Redis database.
    """

    app = fl.Flask(name)

    # Create a redis client and a redis-queue instance
    # Register API blueprint
    app.register_blueprint(api)

    # Can't use per blueprint 404s
    # See http://flask.pocoo.org/docs/0.10/api/#flask.Blueprint.errorhandler
    @app.errorhandler(404)
    def not_found(error):
        return 'Not found', 404

    return app
