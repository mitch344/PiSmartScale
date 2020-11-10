const User = require("../models/user.model.js");

exports.create = (req, res) => {
  if (!req.body) {
    res.status(400).send({
      message: "Content can not be empty!"
    });
  }

  const user = new User({
    name: req.body.name,
    password: req.body.password
  });

  // Save User in the database
  User.create(user, (err, data) => {
    if (err)
      res.status(500).send({
        message:
          err.message || "Some error occurred while creating the User."
      });
    else res.send(data);
  });
};

exports.login = (req, res) => {
  if (!req.body) {
    res.status(400).send({
      message: "Content can not be empty!"
    });
  }

  const user = new User({
    name: req.body.name,
    password: req.body.password
  });

  // Save User in the database
  User.login(user, (err, data) => {
    if (err)
      res.status(500).send({
        message:
          err.message || "Some error occurred while creating the User."
      });
    else res.send(data);
  });
};


exports.getMass = (req, res) => {
    if (!req.body) {
    res.status(400).send({
      message: "Content can not be empty!"
    });
  }

  const user = new User({
    name: req.body.name,
    password: req.body.password,
    mass: req.body.mass
  });

  User.getMass(user, (err, data) => {
    if (err)
      res.status(500).send({
        message:
          err.message || "Error Massdate."
      });
    else res.send(data);
  });
};