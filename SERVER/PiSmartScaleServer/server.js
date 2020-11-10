const express = require("express");
const bodyParser = require("body-parser");

const app = express();

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

app.get("/", (req, res) => {
  res.json({ message: "Server Online.."});
});

require("./app/routes/user.routes.js")(app);

const PORT = process.env.PORT || 1337;
app.listen(PORT, () => {
  console.log("Pi Smart Scale Server v1.0.0");
  console.log(`Port: ${PORT}`);
});