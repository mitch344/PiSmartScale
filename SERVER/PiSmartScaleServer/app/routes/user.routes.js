module.exports = app => {
  const users = require("../controllers/user.controller.js");

  // Create a new instance
  app.post("/users/create", users.create);
  app.post("/users/login", users.login);
  app.post("/users/massdata", users.getMass);
};