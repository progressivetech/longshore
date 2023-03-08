<?php

// This header is here so that IE users that are accessing a contribution or event
// page via an iframe embedded in another web site, will get their cookie/session
// settings set properly even though the iframe is pointing to a different domain
// than the parent page. For more information on what these codes mean, see:
// https://support.ourpowerbase.net/ticket/1575
header('P3P:CP="IDC DSP COR ADM DEVi TAIi PSA PSD IVAi IVDi CONi HIS OUR IND CNT"');
error_reporting(LONG_ERROR_REPORTING);

// All sites have access to extra language fonts.
define("K_PATH_FONTS", "/var/www/powerbase/sites/all/libraries/fonts/");
