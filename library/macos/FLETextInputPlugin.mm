// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#import "FLETextInputPlugin.h"

#import "FLEJSONMethodCodec.h"
#import "FLETextInputModel.h"
#import "FLEViewController+Internal.h"

#include "library/common/internal/text_input_model.h"

#include <map>
#include <iostream>
#include <memory>

static NSString *const kTextInputChannel = @"flutter/textinput";

// See
// https://master-docs-flutter-io.firebaseapp.com/flutter/services/SystemChannels/textInput-constant.html
static NSString *const kSetClientMethod = @"TextInput.setClient";
static NSString *const kShowMethod = @"TextInput.show";
static NSString *const kHideMethod = @"TextInput.hide";
static NSString *const kClearClientMethod = @"TextInput.clearClient";
static NSString *const kSetEditingStateMethod = @"TextInput.setEditingState";
static NSString *const kUpdateEditStateResponseMethod = @"TextInputClient.updateEditingState";
static NSString *const kPerformAction = @"TextInputClient.performAction";
static NSString *const kMultilineInputType = @"TextInputType.multiline";

/**
 * Private properties of FlutterTextInputPlugin.
 */
@interface FLETextInputPlugin () <NSTextInputClient> {
  flutter_desktop_embedding::TextInputModel *model;
}

/**
 * A text input context, representing a connection to the Cocoa text input system.
 */
@property(nonatomic) NSTextInputContext *textInputContext;

/**
 * A dictionary of text input models, one per client connection, keyed
 * by the client connection ID.
 */
@property(nonatomic) NSMutableDictionary<NSNumber *, FLETextInputModel *> *textInputModels;

/**
 * The currently active client connection ID.
 */
@property(nonatomic, nullable) NSNumber *activeClientID;

/**
 * The currently active text input model.
 */
@property(nonatomic, readonly, nullable) FLETextInputModel *activeModel;

/**
 * The channel used to communicate with Flutter.
 */
@property(nonatomic) FLEMethodChannel *channel;

/**
 * The FLEViewController to manage input for.
 */
@property(nonatomic, weak) FLEViewController *flutterViewController;

/**
 * Handles a Flutter system message on the text input channel.
 */
- (void)handleMethodCall:(FLEMethodCall *)call result:(FLEMethodResult)result;

@end

@implementation FLETextInputPlugin

- (instancetype)initWithViewController:(FLEViewController *)viewController {
  self = [super init];
  if (self != nil) {
    _flutterViewController = viewController;
    _channel = [FLEMethodChannel methodChannelWithName:kTextInputChannel
                                       binaryMessenger:viewController
                                                 codec:[FLEJSONMethodCodec sharedInstance]];
    __weak FLETextInputPlugin *weakSelf = self;
    [_channel setMethodCallHandler:^(FLEMethodCall *call, FLEMethodResult result) {
      [weakSelf handleMethodCall:call result:result];
    }];
    _textInputModels = [[NSMutableDictionary alloc] init];
    _textInputContext = [[NSTextInputContext alloc] initWithClient:self];
  }
  return self;
}

#pragma mark - Private

- (FLETextInputModel *)activeModel {
  return (_activeClientID == nil) ? nil : _textInputModels[_activeClientID];
}

/*
 Json::FastWriter fastWritter;
 std::string serialized = fastWritter.write(value);
 serialized.c_str(); // this is the raw byte array (null terminated).
 And Json::Reader to deserialize:

 Json::Reader reader;
 Json::Value value;
 reader.parse(serializedString, value);
 */

std::string parseJson(Json::Value json) {

  // Configure the Builder, then ...
  Json::StreamWriterBuilder wbuilder;
  std::string result = Json::writeString(wbuilder, json);
  return result; // this is the raw byte array (null terminated).
}

- (NSDictionary *)dictFromJsonString:(std::string)string {
  NSString *s = [NSString stringWithCString:string.c_str()
                                              encoding:[NSString defaultCStringEncoding]];
  NSData *data = [s dataUsingEncoding:NSUTF8StringEncoding];
  return [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingAllowFragments error:nil];
}

- (NSString *)stringFromDictionary:(NSDictionary *)dict {
  NSData *data = [NSJSONSerialization dataWithJSONObject:dict options:NSJSONWritingPrettyPrinted error:nil];
  return [[NSString alloc]initWithData:data encoding:NSUTF8StringEncoding];
}

Json::Value jsonFromString(NSString *string) {
  Json::CharReaderBuilder builder;
  Json::CharReader * reader = builder.newCharReader();
  Json::Value result;
  std::string errors;
  std::string s = [string UTF8String];
  bool parsingSuccessful = reader->parse( s.c_str(), s.c_str() + s.size(), &result, &errors);     //parse process
  if (!parsingSuccessful) {
    std::cerr << "Something went wrong" << std::endl;
    std::cerr << errors << std::endl;
  }
  return result;
}

Json::Value convertDict(id dict) {
  Json::Value a;
  if ([dict isKindOfClass:[NSDictionary class]]) {
    for (NSString *key in [dict allKeys]) {
      Json::Value t;
      id v = dict[key];
      if ([v isKindOfClass:[NSString class]]) {
        t = [v UTF8String];
      } else if ([v isKindOfClass:[NSNumber class]]) {
        t = [v intValue];
      } else if ([v isKindOfClass:[NSDictionary class]]){
        t = convertDict(v);
      }
      std::string s = [key UTF8String];
      std::cout << t << std::endl;
      a[s] = t;
    }
  } else if ([dict isKindOfClass:[NSArray class]]) {
    for (int i = 0; i < [dict length]; i++) {
      a.append(convertDict(dict[i]));
    }
  }

  return a;
}

- (void)handleMethodCall:(FLEMethodCall *)call result:(FLEMethodResult)result {
  BOOL handled = YES;
  NSString *method = call.methodName;
  if ([method isEqualToString:kSetClientMethod]) {
    if (!call.arguments[0] || !call.arguments[1]) {
      result([[FLEMethodError alloc]
          initWithCode:@"error"
               message:@"Missing arguments"
               details:@"Missing arguments while trying to set a text input client"]);
      return;
    }
    NSNumber *clientID = call.arguments[0];
    if (clientID != nil &&
        (_activeClientID == nil || ![_activeClientID isEqualToNumber:clientID])) {
      _activeClientID = clientID;
      // TODO: Do we need to preserve state across setClient calls?
      NSString *s = [self stringFromDictionary:call.arguments[1]];
      Json::Value d = jsonFromString(s);
      model = new flutter_desktop_embedding::TextInputModel(d);

      FLETextInputModel *inputModel =
          [[FLETextInputModel alloc] initWithClientID:clientID configuration:call.arguments[1]];
      if (!inputModel) {
        result([[FLEMethodError alloc] initWithCode:@"error"
                                            message:@"Failed to create an input model"
                                            details:@"Configuration arguments might be missing"]);
        return;
      }
      _textInputModels[_activeClientID] = inputModel;
    }
  } else if ([method isEqualToString:kShowMethod]) {
    [self.flutterViewController addKeyResponder:self];
    [_textInputContext activate];
  } else if ([method isEqualToString:kHideMethod]) {
    [self.flutterViewController removeKeyResponder:self];
    [_textInputContext deactivate];
  } else if ([method isEqualToString:kClearClientMethod]) {
    _activeClientID = nil;
  } else if ([method isEqualToString:kSetEditingStateMethod]) {
    NSDictionary *state = call.arguments;
    NSString *s = [self stringFromDictionary:state];

    Json::Value c = jsonFromString(s);
    model->SetEditingState(c);
    self.activeModel.state = state;
    std::cout << model->GetEditingState();
    std::cout << std::endl;
  } else {
    handled = NO;
    NSLog(@"Unhandled text input method '%@'", method);
  }
  result(handled ? nil : FLEMethodNotImplemented);
}

/**
 * Informs the Flutter framework of changes to the text input model's state.
 */
- (void)updateEditState {
  if (self.activeModel == nil) {
    return;
  }
  Json::Value j = model->GetEditingState();
  std::string s = parseJson(j);
  NSDictionary *d = [self dictFromJsonString:s];

  [_channel invokeMethod:kUpdateEditStateResponseMethod
               arguments:@[ _activeClientID, d ]];

//  [_channel invokeMethod:kUpdateEditStateResponseMethod
//               arguments:@[ _activeClientID, _textInputModels[_activeClientID].state ]];
}

#pragma mark -
#pragma mark NSResponder

/**
 * Note, the Apple docs suggest that clients should override essentially all the
 * mouse and keyboard event-handling methods of NSResponder. However, experimentation
 * indicates that only key events are processed by the native layer; Flutter processes
 * mouse events. Additionally, processing both keyUp and keyDown results in duplicate
 * processing of the same keys. So for now, limit processing to just keyDown.
 */
- (void)keyDown:(NSEvent *)event {
  [_textInputContext handleEvent:event];
}

#pragma mark -
#pragma mark NSStandardKeyBindingMethods

/**
 * Note, experimentation indicates that moveRight and moveLeft are called rather
 * than the supposedly more RTL-friendly moveForward and moveBackward.
 */
- (void)moveLeft:(nullable id)sender {
  model->MoveCursorBack();
  std::cout << model->GetEditingState();
  std::cout << std::endl;

  NSRange selection = self.activeModel.selectedRange;
  if (selection.length == 0) {
    if (selection.location > 0) {
      // Move to previous location
      self.activeModel.selectedRange = NSMakeRange(selection.location - 1, 0);
      [self updateEditState];
    }
  } else {
    // Collapse current selection
    self.activeModel.selectedRange = NSMakeRange(selection.location, 0);
    [self updateEditState];
  }
}

- (void)moveRight:(nullable id)sender {
  model->MoveCursorForward();
  std::cout << model->GetEditingState();
  std::cout << std::endl;

  NSRange selection = self.activeModel.selectedRange;
  if (selection.length == 0) {
    if (selection.location < self.activeModel.text.length) {
      // Move to next location
      self.activeModel.selectedRange = NSMakeRange(selection.location + 1, 0);
      [self updateEditState];
    }
  } else {
    // Collapse current selection
    self.activeModel.selectedRange = NSMakeRange(selection.location + selection.length, 0);
    [self updateEditState];
  }
}

- (void)deleteBackward:(id)sender {
  model->Backspace();
  std::cout << model->GetEditingState();
  std::cout << std::endl;

  NSRange selection = self.activeModel.selectedRange;
  if (selection.location == 0) return;
  NSRange range = selection;
  if (selection.length == 0) {
    NSUInteger location = (selection.location == NSNotFound) ? self.activeModel.text.length - 1
                                                             : selection.location - 1;
    range = NSMakeRange(location, 1);
  }
  self.activeModel.selectedRange = NSMakeRange(range.location, 0);
  [self insertText:@"" replacementRange:range];  // Updates edit state
}

#pragma mark -
#pragma mark NSTextInputClient

- (void)insertText:(id)string replacementRange:(NSRange)range {
  model->AddString([string UTF8String]);
  std::cout << model->GetEditingState();
  std::cout << std::endl;

  if (self.activeModel != nil) {
    if (range.location == NSNotFound && range.length == 0) {
      // Use selection
      range = self.activeModel.selectedRange;
    }
    if (range.location > self.activeModel.text.length)
      range.location = self.activeModel.text.length;
    if (range.length > (self.activeModel.text.length - range.location))
      range.length = self.activeModel.text.length - range.location;
    [self.activeModel.text replaceCharactersInRange:range withString:string];
    self.activeModel.selectedRange = NSMakeRange(range.location + ((NSString *)string).length, 0);
    [self updateEditState];
  }
}

- (void)moveUp:(id)sender {
  model->MoveCursorUp();
  std::cout << model->GetEditingState();
  std::cout << std::endl;
  [self updateEditState];
}

- (void)moveDown:(id)sender {
  model->MoveCursorDown();
  std::cout << model->GetEditingState();
  std::cout << std::endl;
  [self updateEditState];
}

- (void)doCommandBySelector:(SEL)selector {
  if ([self respondsToSelector:selector]) {
    // Note: The more obvious [self performSelector...] doesn't give ARC enough information to
    // handle retain semantics properly. See https://stackoverflow.com/questions/7017281/ for more
    // information.
    IMP imp = [self methodForSelector:selector];
    typedef void (*Command)(id, SEL, id);
    Command command = (Command)imp;
    command(self, selector, nil);
  }
}

- (void)insertNewline:(id)sender {
  model->InsertNewLine();
  std::cout << model->GetEditingState();
  std::cout << std::endl;
  [self updateEditState];
//  if ([self.activeModel.inputType isEqualToString:kMultilineInputType]) {
//    [self insertText:@"\n" replacementRange:self.activeModel.selectedRange];
//  }
  [_channel invokeMethod:kPerformAction
               arguments:@[ _activeClientID, self.activeModel.inputAction ]];
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange {
  if (self.activeModel != nil) {
    [self.activeModel.text replaceCharactersInRange:replacementRange withString:string];
    self.activeModel.selectedRange = selectedRange;
    [self updateEditState];
  }
}

- (void)unmarkText {
  if (self.activeModel != nil) {
    self.activeModel.markedRange = NSMakeRange(NSNotFound, 0);
    [self updateEditState];
  }
}

- (NSRange)selectedRange {
  return (self.activeModel == nil) ? NSMakeRange(NSNotFound, 0) : self.activeModel.selectedRange;
}

- (NSRange)markedRange {
  return (self.activeModel == nil) ? NSMakeRange(NSNotFound, 0) : self.activeModel.markedRange;
}

- (BOOL)hasMarkedText {
  return (self.activeModel == nil) ? NO : self.activeModel.markedRange.location != NSNotFound;
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range
                                                actualRange:(NSRangePointer)actualRange {
  if (self.activeModel) {
    if (actualRange != nil) *actualRange = range;
    NSString *substring = [self.activeModel.text substringWithRange:range];
    return [[NSAttributedString alloc] initWithString:substring attributes:nil];
  } else {
    return nil;
  }
}

- (NSArray<NSString *> *)validAttributesForMarkedText {
  return @[];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
  // TODO: Implement.
  // Note: This function can't easily be implemented under the system-message architecture.
  return CGRectZero;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
  // TODO: Implement.
  // Note: This function can't easily be implemented under the system-message architecture.
  return 0;
}

@end
