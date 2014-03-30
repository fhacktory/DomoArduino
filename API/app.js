var express    = require('express');
var app        = express();
var SerialPort = require('serialport').SerialPort;
var mongoose = require('mongoose');
var Schema = mongoose.Schema;
var async = require('async');

mongoose.connect('mongodb://localhost/test');

var EventSchema = new Schema({
    nr: Number,
    src: String,
    port: String,
    dest: String,
    temp: Number,
    door: Boolean,
    battery: Number,
    created_at: { type: Date }
});

EventSchema.pre('save', function(next){
  if ( !this.created_at ) {
    this.created_at = new Date;
  }
  next();
});

var Event = mongoose.model('Event', EventSchema);

var sp = new SerialPort("/dev/ttyACM0", {
   baudRate: 9600,
   dataBits: 8,
   parity: 'none',
   stopBits: 1,
   flowControl: false,
   parser: require('serialport').parsers.readline("\n")
});

function compare (source, target){
    var not_in=new Array;
    var j=0; 
    for(var i=0; i<=source.length-1;i++){
        if(target.indexOf(source[i])==-1){
            not_in[j]=source[i];
            j++;
        }
    }

    return not_in;
}

sp.on('data', function (data) {
    if (data.indexOf(':') < 0) {
        return;
    }

    data = data.split(':');
    var objData = {};

    for (var i in data) {
        data[i] = data[i].split(',');
    }

    data = [].concat.apply([], data);

    for (var i in data) {
        data[i] = data[i].split('=');
        objData[data[i][0]] = data[i][1]
    }

    if (compare(['nr', 'src', 'port','dest', 'temp', 'door','battery'], Object.keys(objData)).length > 0) {
        return;
    }

    Event.find({
            nr: parseInt(objData.nr),
            src: objData.src,
            port: objData.port,
            dest: objData.dest,
            created_at: {"$gte": new Date(Date.now*1000 - 60)}
        },
        function (err, events) {
            if (!err && !events.length) {
                Event.create(objData, function (err, event) {
                    if (err) {
                        console.error(err);
                    }
                });
            }
        });
});

app.get('/networks', function(req, res){
    Event.find().distinct('dest', function (err, networks) {
        var data = [];

        async.concatSeries(
            networks,
            function(network, next) {
                Event.find({"dest": network}).distinct("src", function (err, src) {
                    next(null, {
                        "id": network,
                        "devices": src.length
                    });
                });
            },
            function (err, networks) {
                res.header("Content-Type", "application/json");
                res.send(200, JSON.stringify(networks));
            }
        );
    });
});

app.get('/networks/:id', function(req, res){
    Event.find({"dest": req.params.id}).distinct("src", function (err, src) {
        if (err) {
            throw err;
        }
        res.header("Content-Type", "application/json");
        res.send(200, JSON.stringify({
            "id": req.params.id,
            "devices": src.length
        }));
    });
});

app.get('/networks/:id/devices', function(req, res){
    Event.find({"dest": req.params.id}).distinct('src', function (err, events) {
        if (err) {
            throw err;
        }

        async.concat(
            events,
            function (event, next) {
                Event.find({"src": event, "dest": req.params.id}).count(function (err, count) {
                    next(null, {
                        "id": event,
                        "history": count
                    });
                });
            },
            function (err, devices) {
                res.header("Content-Type", "application/json");
                res.send(200, JSON.stringify(devices));
            }
        );
    });
});

app.get('/networks/:id/history', function(req, res){
    Event.find({"dest": req.params.id}, function (err, events) {
        if (err) {
            throw err;
        }

        res.header("Content-Type", "application/json");
        res.send(200, JSON.stringify(events));
    });
});

app.get('/devices', function(req, res){
    Event.find().distinct('src', function (err, events) {
        if (err) {
            throw err;
        }

        async.concatSeries(
            events,
            function (event, next) {
                Event.find({"src": event}).count(function (err, count) {
                    next(null, {
                        "id": event,
                        "history": count
                    });
                });
            },
            function (err, devices) {
                res.header("Content-Type", "application/json");
                res.send(200, JSON.stringify(devices));
            }
        );
    });
});

app.get('/devices/:id', function(req, res){
    Event.find({"src": req.params.id}).count(function (err, count) {
        res.header("Content-Type", "application/json");
        res.send(200, JSON.stringify({
            "id": req.params.id,
            "history": count
        }));
    });
});

app.get('/devices/:id/history', function(req, res){
    Event.find({"src": req.params.id}, function (err, events) {
        if (err) {
            throw err;
        }

        res.header("Content-Type", "application/json");
        res.send(200, JSON.stringify(events));
    });
});

app.listen(3000);