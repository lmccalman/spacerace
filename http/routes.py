from .api import api
from . import exceptions
import flask as fl
import os
from .response import returns_json
import binascii
from . import algorithms
import json


dispatch = {'whyamispecial': algorithms.why_am_i_special,
            'spatialdetailing': algorithms.spatial_detailing,
            'placeslikeme': algorithms.places_like_me}


def process_job(uid, data_string):
    try:
        job_data = json.loads(data_string.decode())
    except Exception:
        raise exceptions.APIBadRequestException('Invalid json')
    result = dispatch[job_data['algorithm']](uid, job_data['boundaries_name'],
                                             job_data['region_codes'],
                                             job_data["columns"],
                                             job_data['table'],
                                             **job_data['parameters'])
    return result


@api.route('/jobs', methods=['POST'])
@returns_json
def add_job():
    """ Add a new analytics job to the queue"""
    data = fl.request.get_data()  # JSON encoded
    # Generate a random UID
    uid = binascii.b2a_hex(os.urandom(10)).decode()

    # Enqueue a job
    fl.current_app.queue.enqueue_call(process_job, args=(uid, data,),
                                      job_id=uid)

    # Get the full URI
    uri = fl.url_for('.get_job', uid=uid, _external=True)
    status_uri = fl.url_for('.get_status', uid=uid, _external=True)
    return {'uid': uid, 'uri': uri, 'status_uri': status_uri}, 200


@api.route('/jobs/<string:uid>', methods=['GET'])
@returns_json
def get_job(uid):
    """ Get a completed(?) analytics job from the queue"""
    # try:
    job = fl.current_app.queue.fetch_job(uid)
    # except Exception:
    #     raise exceptions.APINotFoundException(
    #             'Cannot find Job with uid {}'.format(uid))

    if job.is_finished:
        return job.result, 200
    else:
        status_uri = fl.url_for('.get_status', uid=uid, _external=True)
        return {"queueing_status": job.get_status(),
                "status_uri": status_uri}, 202


@api.route('/status/<string:uid>', methods=['GET'])
@returns_json
def get_status(uid):
    """ Get a detailed update of the current progress of the job """
    queue_status = fl.current_app.queue.fetch_job(uid).get_status()
    raw_status = fl.current_app.db.get('status:'+uid)
    if raw_status is not None:
        full_status = json.loads(raw_status.decode())
    else:
        full_status = {}
    full_status["queueing_status"] = queue_status
    return full_status, 200

