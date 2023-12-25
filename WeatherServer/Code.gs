// WeatherServer IoT

// Example Google Scrips code to upload data to Google Sheets from Arduino/ESP8266
// Follow setup instructions found here:
// https://github.com/StorageB/Google-Sheets-Logging
// reddit: u/StorageB107
// email: StorageUnitB@gmail.com

// Enter Spreadsheet ID here
var SS = SpreadsheetApp.openById("1xd70BhrFGJPkGkTd92iCzrlmKQmew-sKkNtH4EtRETo");
var str = "";

function doPost(e) {
  var parsedData;
  var result = {};

  try {
    parsedData = JSON.parse(e.postData.contents);
  } catch (f) {
    return ContentService.createTextOutput("Error in parsing request body: " + f.message);
  }

  if (parsedData !== undefined) {
    var flag = parsedData.format;
    if (flag === undefined) {
      flag = 0;
    }

    var sheet = SS.getSheetByName(parsedData.sheet_name); // sheet name to publish data to is specified in Arduino code
    var dataArr = parsedData.values.split(","); // creates an array of the values to publish

    var date_now = Utilities.formatDate(new Date(), "Asia/Karachi", "yyyy-MM-dd"); // gets the current date
    var time_now = Utilities.formatDate(new Date(), "Asia/Karachi", "HH:mm:ss"); // gets the current time

    var value0 = dataArr[0]; // value0 from Arduino code
    var value1 = dataArr[1]; // value1 from Arduino code
    var value2 = dataArr[2]; // value2 from Arduino code
    var value3 = dataArr[3]; // value3 from Arduino code

    // read and execute command from the "payload_base" string specified in Arduino code
    switch (parsedData.command) {
      case "insert_row":
        // sheet.insertRows(2); // insert full row directly below header text

        var range = sheet.getRange("D5:J5"); // use this to insert cells just above the existing data instead of inserting an entire row
        range.insertCells(SpreadsheetApp.Dimension.ROWS); // use this to insert cells just above the existing data instead of inserting an entire row

        sheet.getRange("D5").setValue(date_now); // publish current date to cell A2
        sheet.getRange("E5").setValue("=trunc(d5)"); // publish current date to cell A
        sheet.getRange("F5").setValue(time_now); // publish current time to cell B2
        sheet.getRange("G5").setValue(value0); // publish value0 from Arduino code to cell C2
        sheet.getRange("H5").setValue(value1); // publish value1 from Arduino code to cell D2
        sheet.getRange("I5").setValue(value2); // publish value2 from Arduino code to cell E2
        sheet.getRange("J5").setValue(value3); // publish value3 from Arduino code to cell f2

        str = "Success"; // string to return back to Arduino serial console
        SpreadsheetApp.flush();
        break;

      case "update_cell":
        sheet.getRange(value0).setValue(date_now);

        str = "Success"; // string to return back to Arduino serial console
        SpreadsheetApp.flush();
        break;
    }

    return ContentService.createTextOutput(str);
  } // endif (parsedData !== undefined)
  else {
    return ContentService.createTextOutput("Error! Request body empty or in incorrect format.");
  }
}
