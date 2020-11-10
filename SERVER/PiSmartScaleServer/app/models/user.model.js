const sql = require("./db.js");
const bcrypt = require('bcrypt')

const User = function(user) {
  this.name = user.name;
  this.password = user.password;
  this.mass = user.mass;
};

User.create = async (newUser, result) => {
      const hashedPassword = await bcrypt.hash(newUser.password, 10);
      newUser.password = hashedPassword;
      let querystr0 = "INSERT INTO users (name, password) VALUES (" + sql.escape(newUser.name) + ", " + sql.escape(newUser.password) + ")";
      sql.query(querystr0, (err, res) => {
        if (err){
          result(null, "Failure");
          return;
        }
        result(null, "Success");
      });
    };


//change to login
User.getMass = (prevUser, result) => {

  let querystr0 = "SELECT * FROM users WHERE name = " + sql.escape(prevUser.name);
  sql.query(querystr0, function(err, rows, fields) {

    if(err){
      result(null, "Failure Getting ID");
      return;
    }

    let id = rows[0].id;
    bcrypt.compare(prevUser.password, rows[0].password, function(err, result1) {
      if(result1){
        if(prevUser.mass != undefined){
          if(prevUser.mass > 0){

            let querystr = "SELECT date_time, measurement FROM mass WHERE id = " + sql.escape(id) +" ORDER BY date_time DESC LIMIT " + sql.escape(prevUser.mass);

            sql.query(querystr, (err2, rows2,) =>{
              result(null, rows2);
              });
          }
        }
        else{
          console.log("Undefined");
        }
      }
      else{
      }
    }); 
  });
};

User.login = (prevUser, result) => {
  let querystr = "SELECT * FROM users WHERE name = " + sql.escape(prevUser.name);

  sql.query(querystr, function(err, rows, fields) {
    if (rows == 0){
      result(null, "Failure");
      return;
    }
    let id = rows[0].id;
    bcrypt.compare(prevUser.password, rows[0].password, function(err, result1) {
      if(result1){
        result(null, "Success");
      }
      else{
        result(null, "Failure");
      }
    });
  });
};

module.exports = User;