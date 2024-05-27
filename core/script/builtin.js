/*
    SPDX-FileCopyrightText: 2018 Intevation GmbH <intevation@intevation.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

/* QJSEngine limits what we can do with the global object in C++, so map Doc into this here. */
{
    const props = Object.getOwnPropertyDescriptors(Doc);
    for (prop in props) {
        Object.defineProperty(this, prop, props[prop]);
    }
    for (const name of Object.getOwnPropertyNames(Doc)) {
        if (typeof Doc[name] === 'function') {
            this.__proto__[name] = Doc[name];
        }
    }
}

/* Builtin functions for Okular's PDF JavaScript interpretation. */

/**
 * Merge the last change (of a text field) with the uncommitted change
 */
function AFMergeChange( event )
{
    var start, end;
    if ( event.willCommit ) return event.value;
    start = (event.selStart >= 0) ? event.value.substring(0, event.selStart) : "";
    end = (event.selEnd >= 0 && event.selEnd <= event.value.length) ? event.value.substring(event.selEnd, event.value.length) : "";
    return start + event.change + end;
}


/** AFSimple_Calculate
 *
 * cFunction is a string that identifies the operation.
 *           It is one of AVG, SUM, PRD, MIN, MAX
 * cFields is an array of the names of the fields used to calculate.
 */
function AFSimple_Calculate( cFunction, cFields )
{
    var ret = 0;

    if ( cFunction === "PRD" )
    {
        ret = 1;
    }

    for (i = 0; i < cFields.length; i++)
    {
        var field = Doc.getField( cFields[i] );
        var val = util.stringToNumber( field.value );

        if ( cFunction === "SUM" || cFunction === "AVG" )
        {
            ret += val;
        }
        else if ( cFunction === "PRD" )
        {
            ret *= val;
        }
        else if ( cFunction === "MIN" )
        {
            if ( i === 0 || val < ret )
            {
                ret = val;
            }
        }
        else if ( cFunction === "MAX" )
        {
            if ( i === 0 || val > ret )
            {
                ret = val;
            }
        }
    }

    if ( cFunction === "AVG" )
    {
        ret /= cFields.length;
    }

    event.value = util.numberToString( ret, "g", 32 );
}


/** AFNumber_Format
 *
 * Formats event.value based on parameters.
 *
 * Parameter description based on Acrobat Help:
 *
 * nDec is the number of places after the decimal point.
 *
 * sepStyle is an integer denoting separator style
 *          0 => . as decimal separator   , as thousand separators => 1,234.56
 *          1 => . as decimal separator     no thousand separators => 1234.56
 *          2 => , as decimal separator   . as thousand separators => 1.234,56
 *          3 => , as decimal separator     no thousand separators => 1234,56
 *
 * negStyle is the formatting used for negative numbers: - not implemented.
 * 0 = MinusBlack
 * 1 = Red
 * 2 = ParensBlack
 * 3 = ParensRed
 *
 * currStyle is the currency style - not used.
 *
 * strCurrency is the currency symbol.
 *
 * bCurrencyPrepend is true to prepend the currency symbol;
 *  false to display on the end of the number.
 */
function AFNumber_Format( nDec, sepStyle, negStyle, currStyle, strCurrency, bCurrencyPrepend )
{
    if ( !event.value )
    {
        return;
    }

    var ret;
    var localized = util.stringToNumber( event.value );

    if ( sepStyle === 2 || sepStyle === 3 )
    {
        // Use de_DE as the locale for the dot separator format
        ret = util.numberToString( localized, "f", nDec, 'de_DE' );

        if ( sepStyle === 3 )
        {
            // No thousands separators. Remove all dots from the DE format.
            ret = ret.replace( /\./g, '' );
        }
    }
    else
    {
        // Otherwise US
        ret = util.numberToString( localized, "f", nDec, 'en_US' );

        if ( sepStyle === 1 )
        {
            // No thousands separators. Remove all commas from the US format.
            ret = ret.replace( /,/g, '' );
        }
    }

    if ( strCurrency )
    {
        if ( bCurrencyPrepend )
        {
            ret = strCurrency + ret;
        }
        else
        {
            ret = ret + strCurrency;
        }
    }

    event.value = ret;
}

function AFNumber_Keystroke(nDec, sepStyle, negStyle, currStyle, strCurrency, bCurrencyPrepend)
{
    // TODO
    return;
}

function AFMakeNumber(string)
{
    var type = typeof string;
    if ( type == "number" )
        return string;
    if ( type != "string" )
        return 0;
    return util.stringToNumber( string );
}

/** AFTime_Format
 *
 * Formats event.value based on parameters.
 *
 * Parameter description based on Acrobat Help:
 *
 * ptf is the number which should be used to format the time, is one of:
 * 0 = 24HR_MM [ 14:30 ] 
 * 1 = 12HR_MM [ 2:30 PM ] 
 * 2 = 24HR_MM_SS [ 14:30:15 ] 
 * 3 = 12HR_MM_SS [ 2:30:15 PM ]
 */
function AFTime_Format( ptf )
{
    if( !event.value )
    {
        return;
    }
    var tokens = event.value.split( /\D/ );
    var invalidDate = false;

    // Remove empty elements of the array
    tokens = tokens.filter(Boolean);

    if( tokens.length < 2 )
        invalidDate = true;

    // Check if every number is valid
    for( i = 0 ; i < tokens.length ; ++i )
    {
        if( isNaN( tokens[i] ) )
        {
            invalidDate = true;
            break;
        }
        switch( i )
        {
            case 0:
            {
                if( tokens[i] > 23 || tokens[i] < 0 )
                    invalidDate = true;
                break;
            }
            case 1:
            case 2:
            {
                if( tokens[i] > 59 || tokens[i] < 0 )
                    invalidDate = true;
                break;
            }
        }
    }
    if( invalidDate )
    {
        event.value = "";
        return;
    }

    // Make it of lenght 3, since we use hh, mm, ss
    while( tokens.length < 3 )
        tokens.push( 0 );

    // We get pm string in the user locale to search.
    var dummyPm = util.printd( 'ap', new Date( 2018, 5, 11, 23, 11, 11) ).toLocaleLowerCase();
    // Add 12 to time if it's PM and less than 12
    if( event.value.toLocaleLowerCase().search( dummyPm ) !== -1 && Number( tokens[0] ) < 12 )
        tokens[0] = Number( tokens[0] ) + 12;

    // We use a random date, because we only care about time.
    var date = new Date( 2019, 7, 12, tokens[0], tokens[1], tokens[2] );
    var ret;
    switch( ptf )
    {
        case 0:
            ret = util.printd( "hh:MM", date );
            break;
        case 1:
            ret = util.printd( "h:MM ap", date );
            break;
        case 2:
            ret = util.printd( "hh:MM:ss", date );
            break;
        case 3:
            ret = util.printd( "h:MM:ss ap", date );
            break;
    }
    event.value = ret;
}

/** AFTime_Keystroke
 *
 * Checks if the string in event.value is valid.
 */
function AFTime_Keystroke( ptf )
{
    var completeValue = AFMergeChange(event);
    if ( !completeValue )
    {
        return;
    }
    if ( event.willCommit )
    {
        const COMMIT_TIME_RE = /^(0?[0-9]|1[0-9]|2[0-3]):(0?[0-9]|[1-5][0-9])(:(0?[0-9]|[1-5][0-9]))?(\s*(pm|am))?$/i;
        // HH:MM is required at minimum
        // the seconds are optional to have HH:MM:SS
        // the AM/PM are optional as well
        // Any number of spaces are allowed between the time and AM/PM to allow for compatibility with other pdf viewers.
        event.rc = COMMIT_TIME_RE.test(completeValue);
    }
    else
    {
        // during value entry, check if entry contains only allowed characters.
        const ALLOWED_CHARS = "0-9:ampAMP\\s";
        const ALLOWED_CHARS_TIME_RE = new RegExp(`[^${ALLOWED_CHARS}]`); // match any character not in the character set

        event.rc = !ALLOWED_CHARS_TIME_RE.test(completeValue);
    }
}

/** AFSpecial_Format
 * psf is the type of formatting to use: 
 * 0 = zip code
 * 1 = zip + 4 
 * 2 = phone 
 * 3 = SSN
 *
 * These are all in the US format.
*/
function AFSpecial_Format( psf )
{
    if( !event.value || psf == 0 )
    {
        return;
    }

    var ret = event.value;
    ret = ret.replace(/\D/g, '');
    if( psf === 1 )
        ret = ret.substr( 0, 5 ) + '-' + ret.substr( 5, 4 );

    else if( psf === 2 )
        ret = '(' + ret.substr( 0, 3 ) + ') ' + ret.substr( 3, 3 ) + '-' + ret.substr( 6, 4 );

    else if( psf === 3 )
        ret = ret.substr( 0, 3 ) + '-' + ret.substr( 3, 2 ) + '-' + ret.substr( 5, 4 );

    event.value = ret;
} 

/** AFSpecial_Keystroke
 *
 * Checks if the String in event.value is valid.
 *
 * Parameter description based on Acrobat Help:
 *
 * psf is the type of formatting to use: 
 * 0 = zip code
 * 1 = zip + 4 
 * 2 = phone 
 * 3 = SSN
 *
 * These are all in the US format. We check to see if only numbers are inserted and the length of the string.
*/
function AFSpecial_Keystroke( psf )
{
    var completeValue = AFMergeChange( event );
    if ( !completeValue )
    {
        return;
    }

    const ZIP_NOCOMMIT_RE = /^\d{0,5}$/;
    const ZIP4_NOCOMMIT_RE = /^\d{0,5}?( |\.|-)?\d{0,4}$/;
    const PHONE_NOCOMMIT_RE = /^\(?\d{0,3}\)?( |\.|-)?\d{0,3}( |\.|-)?\d{0,4}$/; // optional "()", ".", "-" and " " are allowed during keystroke for ease of data entry
    const SSN_NOCOMMIT_RE = /^\d{0,3}( |\.|-)?\d{0,2}( |\.|-)?\d{0,4}$/; // optional separators allowed during keystroke
    const ZIP_COMMIT_RE = /^\d{5}$/;    // 12345
    const ZIP4_COMMIT_RE = /^\d{5}( |\.|-)?\d{4}$/; // 12345-6789, 12345 6789, 12345.6789
    const PHONE_COMMIT_RE = /^(\d{3}|\(\d{3}\))( |\.|-)?\d{3}( |\.|-)?\d{4}$/;  // 123 456 7890, (123) 456-7890, 1234567890, 123-456-7890, 123.456-7890, etc
    const SSN_COMMIT_RE = /^\d{3}( |\.|-)?\d{2}( |\.|-)?\d{4}$/;    // 123-45.6789, 123 45 6789, 123456789, 123-45-6789, etc

    var verifyingRe;
    switch( psf )
    {
        // zip code
        case 0:
        {
            verifyingRe = event.willCommit ? ZIP_COMMIT_RE : ZIP_NOCOMMIT_RE;
            break;
        }
        // zip + 4
        case 1:
        {
            verifyingRe = event.willCommit ? ZIP4_COMMIT_RE : ZIP4_NOCOMMIT_RE;
            break;
        }
        // phone
        case 2:
        {
            verifyingRe = event.willCommit ? PHONE_COMMIT_RE : PHONE_NOCOMMIT_RE;
            break;
        }
        // SSN
        case 3:
        {
            verifyingRe = event.willCommit ? SSN_COMMIT_RE : SSN_NOCOMMIT_RE;
        }
    }
    event.rc = verifyingRe.test(completeValue);
}

/** AFPercent_Format
 *
 * Formats event.value in the correct percentage format as per the given parameters
 *
 * Parameter description based on Adobe's documentation and observed behavior in Adobe Reader
 *
 * nDec is the number of places after the decimal point.
 *
 * sepStyle is an integer denoting separator style
 *          0 => . as decimal separator     , as thousand separators => 1,234.56%
 *          1 => . as decimal separator       no thousand separators => 1234.56%
 *          2 => , as decimal separator     . as thousand separators => 1.234,56%
 *          3 => , as decimal separator     no thousand separators => 1234,56
 *          4 => . as decimal separator     ’ as thousand separators => 1’234.56%
 */
function AFPercent_Format( nDec, sepStyle )
{
    if ( !event.value )
    {
        return;
    }

    var ret;
    var percentValue = util.stringToNumber( event.value ) * 100;
    if ( sepStyle === 2 || sepStyle === 3 )
    {
        // Use de_DE as the locale for the dot separator format
        ret = util.numberToString( percentValue, "f", nDec, 'de_DE' );

        if ( sepStyle === 3 )
        {
            // No thousands separators. Remove all dots from the DE format.
            ret = ret.replace( /\./g, '' );
        }
    }
    else if ( sepStyle === 0 || sepStyle === 1 )
    {
        // Otherwise US
        ret = util.numberToString( percentValue, "f", nDec, 'en_US' );

        if ( sepStyle === 1 )
        {
            // No thousands separators. Remove all commas from the US format.
            ret = ret.replace( /,/g, '' );
        }
    }
    else if ( sepStyle === 4 )
    {
        ret = util.numberToString( percentValue, "f", nDec, 'de_CH');
    }
    ret += "%";
    event.value = ret;
}

function AFPercent_Keystroke( nDec, sepStyle )
{
    if (event.willCommit) {
        event.rc = true
    } else {
        // Allow only numbers plus possible separators
        // TODO disallow too many separators
        // TODO Make separator locale-dependen/use sepStyle properly
        event.rc = !isNaN(event.change) || event.change == "." || event.change == ","
    }
}

app.popUpMenuEx = function() {
    return app.okular_popUpMenuEx(arguments);
}

app.popUpMenu = function() {
    // Convert arguments like this:
    //   app.popUpMenu(["Fruits","Apples","Oranges"], "-","Beans","Corn");
    // into this:
    //   app.popUpMenuEx(
    //     {cName:"Fruits", oSubMenu:[
    //       {cName:"Apples"},
    //       {cName:"Oranges"}
    //     ]},
    //     {cName:"-"},
    //     {cName:"Beans"},
    //     {cName:"Corn"}
    //   );
    function convertArgument(arg) {
        var exArguments = [];

        for (element of arg) {
            var newElement = null;

            if (Array.isArray(element) && element.length > 0) {
                newElement = {
                    cName: element[0],
                    oSubMenu: convertArgument(element.slice(1))
                };
            } else if (!Array.isArray(element)) {
                newElement = {
                    cName: element
                };
            }

            if (newElement !== null)
                exArguments.push(newElement);
        }

        return exArguments;
    }

    var exArguments = convertArgument(arguments);
    var result =  app.okular_popUpMenuEx(exArguments);
    return result;
}