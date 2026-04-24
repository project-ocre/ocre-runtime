#!/usr/bin/env python3
# @copyright Copyright (c) contributors to Project Ocre,
# which has been established as Project Ocre a Series of LF Projects, LLC
#
# SPDX-License-Identifier: Apache-2.0

import sys
sys.path.append("../..")

import requests  # noqa: E402
import testlib   # noqa: E402

"""
This testcase is to be used following the flashing of the supervisor sample to a board with the webserver-complex container put up.

The testcase forms a serial connection to the board, runs the hello-world container  
and checks the string "powered by Ocre" appears in the output of the container. 

This testcase makes multiple http GET requests to the webserver-complex server to make sure 
the container is able to process http requests and send back valid responses
"""


def send_get_request(url: str) -> requests.Response:
    print(f"Sending GET request to '{url}'")
    http_response: requests.Response = requests.get(f'{url}')
    if (http_response.status_code != 200):
        print(f"Unable to reach container webserver, got status code {
            http_response.status_code}")
        sys.exit(1)

    return http_response


def main():
    http_response1: requests.Response = send_get_request(
        f'{testlib.bu585_base_url}/api/counter')

    send_get_request(f'{testlib.bu585_base_url}/increment')

    http_response2: requests.Response = send_get_request(
        f'{testlib.bu585_base_url}/api/counter')

    if (http_response2.json().get('counter') < http_response1.json().get('counter')):
        print(f"Remote server did not increment counter correctly, went from {
            http_response1.json().get('counter')} to {http_response2.json().get('counter')}")
        sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
