#!/usr/bin/env bash
#*******************************************************************************
# djinterp [ci]                                                  mongodb_up.sh
#
# Starts the deployments scripts/ci/check_db_mongodb.sh is run against, in
# Docker, from the official `mongo` image of the tag given: a replica set of one
# member, rs0, on 127.0.0.1:27017, which a transaction and a change stream
# need, and a standalone server on 127.0.0.1:27018; neither asks for a
# password. It waits until the set's member is its primary, and prints the
# two connection strings the harnesses read. A workflow's service container
# cannot be given mongod's --replSet, so the containers are started here.
#   The member's host is 127.0.0.1:27017, which is the member itself inside
# its container and the published port outside it, so that the server and
# the harness both reach it by the one name. The shell is the image's
# mongosh, or, before 6.0, its mongo.
#
#   usage:   scripts/ci/mongodb_up.sh <tag>          (e.g. 4.2, 8.0, latest)
#   exit:    0 once both answer, the primary elected; 1 when either does not
#            within two minutes, its log printed
#
# path:      /scripts/ci/mongodb_up.sh
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
#                                                            revised: 2026.10.28
#*******************************************************************************
set -u

tag="${1:?usage: scripts/ci/mongodb_up.sh <tag>}"

# shell <container> <script>
#   runs JavaScript in a container's shell, quietly.
shell() {
    docker exec "$1" sh -c \
        'if command -v mongosh > /dev/null; then s=mongosh; else s=mongo; fi
         "$s" --quiet --eval "$0"' "$2"
}

# wait_for <container> <script> <words>
#   true once the script's output holds the words, within two minutes.
wait_for() {
    for _ in $(seq 60); do
        shell "$1" "$2" 2> /dev/null | grep -q "$3" && return 0
        sleep 2
    done
    docker logs "$1" 2>&1 | tail -n 30
    return 1
}

docker run -d --name djinterp_rs -p 27017:27017 "mongo:$tag" \
    --replSet rs0 --bind_ip_all > /dev/null || exit 1
docker run -d --name djinterp_alone -p 27018:27017 "mongo:$tag" \
    --bind_ip_all > /dev/null || exit 1

wait_for djinterp_rs "db.adminCommand({ping: 1}).ok" 1 || exit 1
shell djinterp_rs \
    "rs.initiate({_id: 'rs0', members: [{_id: 0, host: '127.0.0.1:27017'}]})"
wait_for djinterp_rs "db.isMaster().ismaster" true || exit 1
wait_for djinterp_alone "db.adminCommand({ping: 1}).ok" 1 || exit 1

echo "DJINTERP_TEST_MONGODB_URI=mongodb://127.0.0.1:27017/?replicaSet=rs0"
echo "DJINTERP_TEST_MONGODB_STANDALONE_URI=mongodb://127.0.0.1:27018/"
