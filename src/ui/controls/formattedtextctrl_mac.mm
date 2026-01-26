///////////////////////////////////////////////////////////////////////////////
// Name:        formattedtextctrl_mac.mm
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "formattedtextctrl_mac.h"

#ifdef __WXOSX__

#import <Cocoa/Cocoa.h>
#include <wx/osx/private.h>

// custom NSTextView subclass that provides headers and footers during printing
@interface TextViewForPrinting : NSTextView
{
@public
    NSMutableAttributedString* m_pageHeader;
    NSMutableAttributedString* m_pageFooter;

    NSString* m_watermarkText;
    CGFloat   m_watermarkOpacity;
    CGFloat   m_watermarkAngle;
    NSColor* m_watermarkColor;
}

- (NSAttributedString*)pageHeader;
- (NSAttributedString*)pageFooter;

@end

@implementation TextViewForPrinting

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self != nil)
        {
        m_pageHeader = nil;
        m_pageFooter = nil;

        m_watermarkText = nil;
        m_watermarkOpacity = 0.5;
        m_watermarkAngle = -45.0;
        m_watermarkColor = nil;
        }
    return self;
}

- (void)dealloc
{
    [m_pageHeader release];
    [m_pageFooter release];
    [m_watermarkColor release];
    [m_watermarkText release];
    [super dealloc];
}

- (NSFont*)headerFooterFont
{
    NSFont* font = [NSFont systemFontOfSize:12];
    return font;
}

- (NSAttributedString*)pageHeader
{
    if (m_pageHeader == nil)
        {
        return nil;
        }

    NSMutableString* formattedHeader =
        [[[NSMutableString alloc] initWithString:[m_pageHeader string]] autorelease];

    // replace @PN with current page number
    NSString* pageNumberStr =
        [NSString stringWithFormat:@"%lu",
            static_cast<unsigned long>([[NSPrintOperation currentOperation] currentPage])];
    [formattedHeader replaceOccurrencesOfString:@"@PN"
                                     withString:pageNumberStr
                                        options:0
                                          range:NSMakeRange(0, [formattedHeader length])];

    // replace @PC with total page count
    NSRange pageRange = [[NSPrintOperation currentOperation] pageRange];
    NSString* pageCountStr =
        [NSString stringWithFormat:@"%lu", static_cast<unsigned long>(pageRange.length)];
    [formattedHeader replaceOccurrencesOfString:@"@PC"
                                     withString:pageCountStr
                                        options:0
                                          range:NSMakeRange(0, [formattedHeader length])];

    NSMutableAttributedString* attributedFormattedHeader =
        [[[NSMutableAttributedString alloc] initWithString:formattedHeader] autorelease];
    [attributedFormattedHeader addAttribute:NSFontAttributeName
                                      value:[self headerFooterFont]
                                      range:NSMakeRange(0, [attributedFormattedHeader length])];

    // handle tabs as center/right
    CGFloat pageWidth = [[NSPrintOperation currentOperation].printInfo paperSize].width;
    NSMutableParagraphStyle* style = [[[NSMutableParagraphStyle alloc] init] autorelease];

    // center tab at half width, right tab near right margin
    NSTextTab* centerTab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentCenter
                                                         location:pageWidth/2
                                                           options:@{}];
    NSTextTab* rightTab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentRight
                                                        location:pageWidth-36  // 36 pt right margin
                                                          options:@{}];
    [style setTabStops:@[centerTab, rightTab]];

    [attributedFormattedHeader addAttribute:NSParagraphStyleAttributeName
                                      value:style
                                      range:NSMakeRange(0, [attributedFormattedHeader length])];

    return attributedFormattedHeader;
}

- (NSAttributedString*)pageFooter
{
    if (m_pageFooter == nil)
        {
        return nil;
        }

    NSMutableString* formattedFooter =
        [[[NSMutableString alloc] initWithString:[m_pageFooter string]] autorelease];

    // replace @PN with current page number
    NSString* pageNumberStr =
        [NSString stringWithFormat:@"%lu",
            static_cast<unsigned long>([[NSPrintOperation currentOperation] currentPage])];
    [formattedFooter replaceOccurrencesOfString:@"@PN"
                                     withString:pageNumberStr
                                        options:0
                                          range:NSMakeRange(0, [formattedFooter length])];

    // replace @PC with total page count
    NSRange pageRange = [[NSPrintOperation currentOperation] pageRange];
    NSString* pageCountStr =
        [NSString stringWithFormat:@"%lu", static_cast<unsigned long>(pageRange.length)];
    [formattedFooter replaceOccurrencesOfString:@"@PC"
                                     withString:pageCountStr
                                        options:0
                                          range:NSMakeRange(0, [formattedFooter length])];

    NSMutableAttributedString* attributedFormattedFooter =
        [[[NSMutableAttributedString alloc] initWithString:formattedFooter] autorelease];
    [attributedFormattedFooter addAttribute:NSFontAttributeName
                                      value:[self headerFooterFont]
                                      range:NSMakeRange(0, [attributedFormattedFooter length])];

    // handle tabs as center/right
    CGFloat pageWidth = [[NSPrintOperation currentOperation].printInfo paperSize].width;
    NSMutableParagraphStyle* style = [[[NSMutableParagraphStyle alloc] init] autorelease];

    NSTextTab* centerTab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentCenter
                                                         location:pageWidth/2
                                                           options:@{}];
    NSTextTab* rightTab = [[NSTextTab alloc] initWithTextAlignment:NSTextAlignmentRight
                                                        location:pageWidth-36
                                                          options:@{}];
    [style setTabStops:@[centerTab, rightTab]];

    [attributedFormattedFooter addAttribute:NSParagraphStyleAttributeName
                                      value:style
                                      range:NSMakeRange(0, [attributedFormattedFooter length])];

    return attributedFormattedFooter;
}

- (void)drawRect:(NSRect)rect
{
    // draw the text content first
    [super drawRect:rect];

    // then draw watermark on top (only if we have watermark text)
    if (m_watermarkText == nil || [m_watermarkText length] == 0)
        {
        return;
        }

    NSGraphicsContext* ctx = [NSGraphicsContext currentContext];
    [ctx saveGraphicsState];

    CGContextRef cg = [ctx CGContext];

    // center watermark in the visible rect
    NSPoint center = NSMakePoint(NSMidX(rect), NSMidY(rect));

    CGContextTranslateCTM(cg, center.x, center.y);
    CGContextRotateCTM(cg, m_watermarkAngle * M_PI / 180.0);
    CGContextTranslateCTM(cg, -center.x, -center.y);

    NSColor* baseColor = (m_watermarkColor != nil) ? m_watermarkColor : [NSColor blackColor];
    NSDictionary* attrs = @{
    NSFontAttributeName :
        [NSFont boldSystemFontOfSize:72],
    NSForegroundColorAttributeName :
        [baseColor colorWithAlphaComponent:m_watermarkOpacity]
    };

    NSSize textSize = [m_watermarkText sizeWithAttributes:attrs];
    NSPoint drawPoint = NSMakePoint(
        center.x - (textSize.width / 2.0),
        center.y - (textSize.height / 2.0));

    [m_watermarkText drawAtPoint:drawPoint withAttributes:attrs];

    [ctx restoreGraphicsState];
}

@end

namespace Wisteria::UI
    {
    //------------------------------------------------------------------
    void macOSPrintRTF(const wxString& rtfContent,
                       const wxSize& paperSize,
                       int orientation,
                       const wxString& header,
                       const wxString& footer,
                       const Canvas::Watermark& watermark)
        {
        @autoreleasepool
            {
            // set up print info
            NSPrintInfo* const sharedInfo = [NSPrintInfo sharedPrintInfo];
            NSMutableDictionary* const sharedDict = [sharedInfo dictionary];
            NSMutableDictionary* const printInfoDict =
                [NSMutableDictionary dictionaryWithDictionary:sharedDict];

            // enable header and footer functionality
            [printInfoDict setValue:[NSNumber numberWithBool:YES]
                             forKey:NSPrintHeaderAndFooter];

            NSPrintInfo* const printInfo =
                [[[NSPrintInfo alloc] initWithDictionary:printInfoDict] autorelease];

            // set margins (36 points = 0.5 inch)
            [printInfo setTopMargin:36];
            [printInfo setBottomMargin:36];
            [printInfo setLeftMargin:36];
            [printInfo setRightMargin:36];

            // set paper size and orientation
            const NSSize nsPaperSize = {
                static_cast<CGFloat>(paperSize.GetWidth()),
                static_cast<CGFloat>(paperSize.GetHeight())
            };
            [printInfo setPaperSize:nsPaperSize];
            [printInfo setHorizontalPagination:NSPrintingPaginationModeFit];
            [printInfo setVerticalPagination:NSPrintingPaginationModeAutomatic];
            [printInfo setOrientation:(orientation == wxPORTRAIT) ?
                NSPaperOrientationPortrait : NSPaperOrientationLandscape];
            [printInfo setVerticallyCentered:NO];

            // create print view
            TextViewForPrinting* const printView =
                [[[TextViewForPrinting alloc]
                    initWithFrame:NSMakeRect(0.0, 0.0, nsPaperSize.width, nsPaperSize.height)]
                    autorelease];

            // set up headers and footers
            if (!header.empty())
                {
                printView->m_pageHeader =
                    [[NSMutableAttributedString alloc]
                        initWithString:wxCFStringRef(header).AsNSString()];
                }
            else
                {
                printView->m_pageHeader = nil;
                }

            if (!footer.empty())
                {
                printView->m_pageFooter =
                    [[NSMutableAttributedString alloc]
                        initWithString:wxCFStringRef(footer).AsNSString()];
                }
            else
                {
                printView->m_pageFooter = nil;
                }

            if (watermark.m_color.IsOk())
                {
                printView->m_watermarkColor =
                    [[NSColor colorWithCalibratedRed:
                        watermark.m_color.Red()   / 255.0
                        green: watermark.m_color.Green() / 255.0
                        blue:  watermark.m_color.Blue()  / 255.0
                        alpha: watermark.m_color.Alpha() / 255.0] retain];
                }

            printView->m_watermarkText = [wxCFStringRef(watermark.m_label).AsNSString() retain];
            printView->m_watermarkOpacity = 0.5;
            if (watermark.m_direction == Canvas::WatermarkDirection::Diagonal)
                {
                printView->m_watermarkAngle = -45.0;
                }
            else
                {
                printView->m_watermarkAngle = 0;
                }

            // convert RTF string to NSData and load into the text view
            // RTF should be 7-bit ASCII
            const wxScopedCharBuffer rtfBuffer = rtfContent.ToUTF8();
            NSData* const rtfData = [NSData dataWithBytes:rtfBuffer.data()
                                             length:rtfBuffer.length()];

            const NSRange printViewRange = NSMakeRange(0, [[printView textStorage] length]);
            [printView replaceCharactersInRange:printViewRange withRTF:rtfData];

            // run the print operation
            NSPrintOperation* const printOp =
                [NSPrintOperation printOperationWithView:printView printInfo:printInfo];
            [printOp runOperation];
            // printView is autoreleased; its dealloc will clean up m_pageHeader and m_pageFooter
            }
        }
    } // namespace Wisteria::UI

#endif // __WXOSX__
