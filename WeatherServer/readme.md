Google Setup

1.  Get a google account
2.  Make a new GoogleSheet named "WeatherServer"
3.  Open the GoogleSheet and in the url bar select and copy the GoogleSheet ID. It is the 44 characters after the /d/ part of the URL.
    E.g in https://docs.google.com/spreadsheets/d/1yd3usWODhiEvPAR54bU-i6xDjtMUMItjgWRe7tYVwb0/edit#gid=0,
    the ID is 1yd3usWODhiEvPAR54bU-i6xDjtMUMItjgWRe7tYVwb0.
4.  In the Google Sheet menu, select Extensions > AppScript on the menu toolbar
5.  In the Code.gs tab, enter code for WebApp (Code.gs file)
6.  In the code, paset the GoogleScript ID obtained in Step 3 between the quotes in the line at
    var SS = SpreadsheetApp.openById('1eeeeeeeeeeeeeeeee2iCzrlmKQmew-sKkNtH4EtRETo');
7.  Save, then click on Deploy button > New Deployment
8.  Select "Web App" in type of deployment
9.  Enter
    Description: "WeatherServer",
    WebApp Execute as: "Me",
    Who has access: "Anyone"
10. Authorize access to data, and select your google account.
11. Google will warn that it hasn't verified the app. Click on Advanced, and click on "Go to Untitled project (unsafe)"
12. Click "Allow"
13. Copy Delpoyment ID. You will need to paste this in your Arduino code between quotes at...
    const char \*GScriptId = "AeeeeeeeeeeeeeeenSuuGrBx1bWgUoSByAzwDQnij2PXeG2-j3ZwIRmuPZRkj4fRsFUOAED";
    Remember that this ID will change for each New Deployment ie new version of code.
14. Rename the existing Sheet, or make a new Sheet named "Data". This is where the data will be written.

Arduino Setup

15. Paste deployment ID as in step 13.
16. Enter Wifi credentials in the file WifiCredentials.h
17. Compile
