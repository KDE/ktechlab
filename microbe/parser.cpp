/***************************************************************************
 *   Copyright (C) 2004-2005 by Daniel Clarke                              *
 *   daniel.jc@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "btreebase.h"
#include "expression.h"
#include "instruction.h"
#include "parser.h"
#include "pic14.h"
#include "traverser.h"

#include <cassert>
#include <QDebug>
#include <QFile>
#include <QRegExp>
#include <QRegularExpression>
#include <QString>

#include <iostream>

using namespace std;

using Qt::StringLiterals::operator""_L1;

//BEGIN class Parser
Parser::Parser( MicrobeApp * _mb )
{
	m_code = nullptr;
	m_pPic = nullptr;
	mb = _mb;
	// Set up statement definitions.
	StatementDefinition definition;

	definition.append( Field(Field::Label, "label"_L1) );
	m_definitionMap["goto"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Label, "label"_L1) );
	m_definitionMap["call"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Expression, "expression"_L1) );
	definition.append( Field(Field::Code, "code"_L1) );
	m_definitionMap["while"_L1] = definition;
	definition.clear();

	m_definitionMap["end"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Label, "label"_L1) );
	definition.append( Field(Field::Code, "code"_L1) );
	// For backwards compatibility
	m_definitionMap["sub"_L1] = definition;
	m_definitionMap["subroutine"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Label, "label"_L1) );
	definition.append( Field(Field::Code, "code"_L1) );
	m_definitionMap["interrupt"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Label, "alias"_L1) );
	definition.append( Field(Field::Label, "dest"_L1) );
	m_definitionMap["alias"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Expression, "expression"_L1) );
	definition.append( Field(Field::FixedString, nullptr, "then", true) );
	definition.append( Field(Field::Code, "ifCode"_L1) );
	definition.append( Field(Field::Newline) );
	definition.append( Field(Field::FixedString, nullptr, "else", false) );
	definition.append( Field(Field::Code, "elseCode"_L1) );
	m_definitionMap["if"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Expression, "initExpression"_L1) );
	definition.append( Field(Field::FixedString, nullptr, "to", true) );
	definition.append( Field(Field::Expression, "toExpression"_L1) );
	definition.append( Field(Field::FixedString, nullptr, "step", false) );
	definition.append( Field(Field::Expression, "stepExpression"_L1) );
	definition.append( Field(Field::Code, "code"_L1) );
	m_definitionMap["for"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Variable, "variable"_L1) );
	m_definitionMap["decrement"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Variable, "variable"_L1) );
	m_definitionMap["increment"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Variable, "variable"_L1) );
	m_definitionMap["rotateleft"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Variable, "variable"_L1) );
	m_definitionMap["rotateright"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Code, "code"_L1) );
	m_definitionMap["asm"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Expression, "expression"_L1) );
	m_definitionMap["delay"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Code, "code"_L1) );
	definition.append( Field(Field::Newline) );
	definition.append( Field(Field::FixedString, nullptr, "until", true) );
	definition.append( Field(Field::Expression, "expression"_L1) );
	m_definitionMap["repeat"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Name, "name"_L1) );
	definition.append( Field(Field::PinList, "pinlist"_L1) );
	m_definitionMap["sevenseg"_L1] = definition;
	definition.clear();

	definition.append( Field(Field::Name, "name"_L1) );
	definition.append( Field(Field::PinList, "pinlist"_L1) );
	m_definitionMap["keypad"_L1] = definition;
	definition.clear();
}

Parser::~Parser()
{
}

Parser* Parser::createChildParser()
{
	Parser * parser = new Parser( mb );

	return parser;
}


Code * Parser::parseWithChild( const SourceLineList & lines )
{
	Parser * p = createChildParser();
	Code * code = p->parse(lines);
	delete p;
	return code;
}


Code * Parser::parse( const SourceLineList & lines )
{
	StatementList sList;
	m_pPic = mb->makePic();
	m_code = new Code();
	m_pPic->setCode( m_code );
	m_pPic->setParser(this);
	m_bPassedEnd = false;

	/* First pass
	   ==========

	   Here we go through the code making each line into a statement object,
	   looking out for braced code as we go, if we find it then we put then
	   we make attach the braced code to the statement.
	*/

	SourceLineList::const_iterator end = lines.end();
	for ( SourceLineList::const_iterator slit = lines.begin(); slit != end; ++slit )
	{
		Statement s;
		s.content = *slit;

		// Check to see if the line after next is a brace
		SourceLineList::const_iterator previous = slit;
		if ( (++slit != end) && (*slit).text() == "{"_L1 )
			s.bracedCode = getBracedCode( & slit, end );
		else
			slit = previous;

		if ( !s.text().isEmpty() )
			sList.append(s);
	}

	mb->resetDest();

	for( StatementList::Iterator sit = sList.begin(); sit != sList.end(); ++sit )
	{
		m_currentSourceLine = (*sit).content;

		QString line = (*sit).text();

		QString command; // e.g. "delay", "for", "subroutine", "increment", etc
		{
			int spacepos = line.indexOf(QLatin1Char(' '));
			if ( spacepos >= 0 )
				command = line.left( spacepos );
			else
				command = line;
		}
		OutputFieldMap fieldMap;

		if ( (*sit).content.line() >= 0 )
			m_code->append( InstructionPtr(new Instr_sourceCode( QString(QLatin1StringView("#MSRC\t%1; %2\t%3"))
					.arg( (*sit).content.line() + 1 ).arg( (*sit).content.url() ).arg( (*sit).content.text() ) )));
		bool showBracesInSource = (*sit).hasBracedCode();
		if ( showBracesInSource )
			m_code->append(InstructionPtr(new Instr_sourceCode("{"_L1)));

		// Use the first token in the line to look up the statement type
		DefinitionMap::Iterator dmit = m_definitionMap.find(command);
		if(dmit == m_definitionMap.end())
		{
			if( !processAssignment( (*sit).text() ) )
			{
				// Not an assignment, maybe a label
				if( (*sit).isLabel() )
				{
					QString label = (*sit).text().left( (*sit).text().length() - 1 );
					///TODO sanity check label name and then do error like "Bad label"
					m_pPic->Slabel( label );
				}
				else
					mistake( MicrobeApp::MicrobeApp::UnknownStatement );
			}

			continue; // Give up on the current statement
		}
		StatementDefinition definition = dmit.value();

		// Start at the first white space character following the statement name
		int newPosition = 0;
		int position = command.length() + 1;

		// Temporaries for use inside the switch
		Field nextField;
		Statement nextStatement;

		bool errorInLine = false;
		bool finishLine = false;

		for( StatementDefinition::Iterator sdit = definition.begin(); sdit != definition.end(); ++sdit )
		{
			// If there is an error, or we have finished the statement,
			// the stop. If we are at the end of a line in a multiline, then
			// break to fall through to the next line
			if( errorInLine || finishLine) break;

			Field field = (*sdit);
			QString token;

			bool saveToken = false;
			bool saveBraced = false;
			bool saveSingleLine = false;

			switch(field.type())
			{
				case (Field::Label):
				case (Field::Variable):
				case (Field::Name):
				{
					newPosition = line.indexOf( ' ', position );
					if(position == newPosition)
					{
						newPosition = -1;
						token = line.mid(position);
					}
					else token = line.mid(position, newPosition - position);
					if( token.isEmpty() )
					{
						if(field.type() == Field::Label)
							mistake( MicrobeApp::MicrobeApp::LabelExpected );
						else if (field.type() == Field::Variable)
							mistake( MicrobeApp::VariableExpected );
						else // field.type() == Field::Name
							mistake( MicrobeApp::NameExpected );
						errorInLine = true;
						continue;
					}
					position = newPosition;
					saveToken = true;
					break;
				}

				case (Field::Expression):
				{
					// This is slightly different, as there is nothing
					// in particular that delimits an expression, we just have to
					// look at what comes next and hope we can use that.
					StatementDefinition::Iterator it(sdit);
					++it;
					if( it != definition.end() )
					{
						nextField = (*it);
						if(nextField.type() == Field::FixedString)
							newPosition = line.indexOf(QRegularExpression("\\b"_L1 + nextField.string() + "\\b"_L1));
						// Although code is not necessarily braced, after an expression it is the only
						// sensilbe way to have it.
						else if(nextField.type() == Field::Code)
						{
							newPosition = line.indexOf("{"_L1);
							if(newPosition == -1) newPosition = line.length() + 1;
						}
						else if(nextField.type() == Field::Newline)
							newPosition = line.length()+1;
						else qDebug() << "Bad statement definition - awkward field type after expression";
					}
					else newPosition = line.length() + 1;
					if(newPosition == -1)
					{
						// Something was missing, we'll just play along for now,
						// the next iteration will catch whatever was supposed to be there
					}
					token = line.mid(position, newPosition - position);
					position = newPosition;
					saveToken = true;
				}
				break;

				case (Field::PinList):
				{
					// For now, just assume that the list of pins will continue to the end of the tokens.
					// (we could check until we come across a non-pin, but no command has that format at
					// the moment).

					token = line.mid( position + 1 );
					position = line.length() + 1;
					if ( token.isEmpty() )
						mistake( MicrobeApp::PinListExpected );
					else
						saveToken = true;

					break;
				}

				case (Field::Code):
					if ( !(*sit).hasBracedCode() )
					{
						saveSingleLine = true;
						token = line.mid(position);
						position = line.length() + 1;
					}
					else if( position != -1  && position <= int(line.length()) )
					{
						mistake( MicrobeApp::UnexpectedStatementBeforeBracket );
						errorInLine = true;
						continue;
					}
					else
					{
						// Because of the way the superstructure parsing works there is no
						// 'next line' as it were, the braced code is attached to the current line.
						saveBraced = true;
					}
					break;

				case (Field::FixedString):
				{
					// Is the string found, and is it starting in the right place?
					int stringPosition  = line.indexOf(QRegularExpression("\\b"+field.string()+"\\b"_L1));
					if( stringPosition != position || stringPosition == -1 )
					{
						if( !field.compulsory() )
						{
							position = -1;
							// Skip the next field
							++sdit;
							continue;
						}
						else
						{
							// Otherwise raise an appropriate error
							mistake( MicrobeApp::FixedStringExpected, field.string() );
							errorInLine = true;
							continue;
						}
					}
					else
					{
						position += field.string().length() + 1;
					}
				}
					break;

				case (Field::Newline):
					// It looks like the best way to handle this is to just actually
					// look at the next line, and see if it begins with an expected fixed
					// string.

					// Assume there is a next field, it would be silly if there weren't.
					nextField = *(++StatementDefinition::Iterator(sdit));
					if( nextField.type() == Field::FixedString )
					{
						nextStatement = *(++StatementList::Iterator(sit));
						newPosition = nextStatement.text().indexOf(QRegularExpression("\\b"_L1 + nextField.string() + "\\b"_L1));
						if(newPosition != 0)
						{
							// If the next field is optional just carry on as nothing happened,
							// the next line will be processed as a new statement
							if(!nextField.compulsory()) continue;

						}
						position = 0;
						line = (*(++sit)).text();
						m_currentSourceLine = (*sit).content;
					}

					break;

				case (Field::None):
					// Do nothing
					break;
			}

			if ( saveToken )
				fieldMap[field.key()] = OutputField( token );

			if ( saveSingleLine )
			{
				SourceLineList list;
				list << SourceLineMicrobe( token, nullptr, -1 );
				fieldMap[field.key()] = OutputField( list );
			}

			if ( saveBraced )
				fieldMap[field.key()] = OutputField( (*sit).bracedCode );
			// If position = -1, we have reached the end of the line.
		}

		// See if we got to the end of the line, but not all fields had been
		// processed.
		if( position != -1  && position <= int(line.length()) )
		{
			mistake( MicrobeApp::TooManyTokens );
			errorInLine = true;
		}

		if( errorInLine ) continue;

		// Everything has been parsed up, so send it off for processing.
		processStatement( command, fieldMap );

		if( showBracesInSource )
			m_code->append(InstructionPtr(new Instr_sourceCode("}"_L1)));
	}

	delete m_pPic;
	return m_code;
}

bool Parser::processAssignment(const QString &line)
{
	QStringList tokens = Statement::tokenise(line);

	// Have to have at least 3 tokens for an assignment;
	if ( tokens.size() < 3 )
		return false;

	QString firstToken = tokens[0];

	firstToken = mb->alias(firstToken);
	// Well firstly we look to see if it is a known variable.
	// These can include 'special' variables such as ports
	// For now, the processor subclass generates ports it self
	// and puts them in a list for us.


	// Look for port variables first.
	if ( firstToken.contains("."_L1) )
	{
		PortPin portPin = m_pPic->toPortPin( firstToken );

		// check port is valid
		if ( portPin.pin() == -1 )
			mistake( MicrobeApp::InvalidPort, firstToken );
		// more error checking
		if ( tokens[1] != "="_L1 )
			mistake( MicrobeApp::UnassignedPin );

		QString state = tokens[2];
		if( state == "high"_L1 )
			m_pPic->Ssetlh( portPin, true );
		else if( state == "low"_L1 )
			m_pPic->Ssetlh( portPin, false );
		else
			mistake( MicrobeApp::NonHighLowPinState );
	}
	// no dots, lets try for just a port name
	else if( m_pPic->isValidPort( firstToken ) )
	{
		// error checking
		if ( tokens[1] != "="_L1 )
			mistake( MicrobeApp::UnassignedPort, tokens[1] );

		Expression( m_pPic, mb, m_currentSourceLine, false ).compileExpression(line.mid(line.indexOf("="_L1)+1));
		m_pPic->saveResultToVar( firstToken );
	}
	else if ( m_pPic->isValidTris( firstToken ) )
	{
		if( tokens[1] == "="_L1 )
		{
			Expression( m_pPic, mb, m_currentSourceLine, false ).compileExpression(line.mid(line.indexOf("="_L1)+1));
			m_pPic->Stristate(firstToken);
		}
	}
	else
	{
		// Is there an assignment?
		if ( tokens[1] != "="_L1 )
			return false;

		if ( !mb->isValidVariableName( firstToken ) )
		{
			mistake( MicrobeApp::InvalidVariableName, firstToken );
			return true;
		}

		// Don't care whether or not the variable is new; MicrobeApp will only add it if it
		// hasn't been defined yet.
		mb->addVariable( Variable( Variable::charType, firstToken ) );

		Expression( m_pPic, mb, m_currentSourceLine, false ).compileExpression(line.mid(line.indexOf("="_L1)+1));

		Variable v = mb->variable( firstToken );
		switch ( v.type() )
		{
			case Variable::charType:
				m_pPic->saveResultToVar( v.name() );
				break;

			case Variable::keypadType:
				mistake( MicrobeApp::ReadOnlyVariable, v.name() );
				break;

			case Variable::sevenSegmentType:
				m_pPic->SsevenSegment( v );
				break;

			case Variable::invalidType:
				// Doesn't happen, but include this case to avoid compiler warnings
				break;
		}
	}

	return true;
}


SourceLineList Parser::getBracedCode( SourceLineList::const_iterator * it, SourceLineList::const_iterator end )
{
	// Note: The sourceline list has the braces on separate lines.

	// This function should only be called when the parser comes across a line that is a brace.
	assert( (**it).text() == "{"_L1 );

	SourceLineList braced;

	// Jump past the first brace
	unsigned level = 1;
	++(*it);

	for ( ; *it != end; ++(*it) )
	{
		if ( (**it).text() == "{"_L1 )
			level++;

		else if ( (**it).text() == "}"_L1 )
			level--;

		if ( level == 0 )
			return braced;

		braced << **it;
	}

	// TODO Error: mismatched bracing
	return braced;
}


void Parser::processStatement( const QString & name, const OutputFieldMap & fieldMap )
{
	// Name is guaranteed to be something known, the calling
	// code has taken care of that. Also fieldMap is guaranteed to contain
	// all required fields.

	if ( name == "goto"_L1 )
		m_pPic->Sgoto(fieldMap["label"_L1].string());

	else if ( name == "call"_L1 )
		m_pPic->Scall(fieldMap["label"_L1].string());

	else if ( name == "while"_L1 )
		m_pPic->Swhile( parseWithChild(fieldMap["code"_L1].bracedCode() ), fieldMap["expression"_L1].string() );

	else if ( name == "repeat"_L1 )
		m_pPic->Srepeat( parseWithChild(fieldMap["code"_L1].bracedCode() ), fieldMap["expression"_L1].string() );

	else if ( name == "if"_L1 )
		m_pPic->Sif(
				parseWithChild(fieldMap["ifCode"_L1].bracedCode() ),
				parseWithChild(fieldMap["elseCode"_L1].bracedCode() ),
				fieldMap["expression"_L1].string() );

	else if ( name == "sub"_L1 || name == "subroutine"_L1 )
	{
		if(!m_bPassedEnd)
		{
			mistake( MicrobeApp::InterruptBeforeEnd );
		}
		else
		{
			m_pPic->Ssubroutine( fieldMap["label"_L1].string(), parseWithChild( fieldMap["code"_L1].bracedCode() ) );
		}
	}
	else if( name == "interrupt"_L1 )
	{
		QString interrupt = fieldMap["label"_L1].string();

		if(!m_bPassedEnd)
		{
			mistake( MicrobeApp::InterruptBeforeEnd );
		}
		else if( !m_pPic->isValidInterrupt( interrupt ) )
		{
			mistake( MicrobeApp::InvalidInterrupt );
		}
		else if ( mb->isInterruptUsed( interrupt ) )
		{
			mistake( MicrobeApp::InterruptRedefined );
		}
		else
		{
			mb->setInterruptUsed( interrupt );
			m_pPic->Sinterrupt( interrupt, parseWithChild( fieldMap["code"_L1].bracedCode() ) );
		}
	}
	else if( name == "end"_L1 )
	{
		///TODO handle end if we are not in the top level
		m_bPassedEnd = true;
		m_pPic->Send();
	}
	else if( name == "for"_L1 )
	{
		QString step = fieldMap["stepExpression"_L1].string();
		bool stepPositive;

		if( fieldMap["stepExpression"_L1].found() )
		{
			if(step.left(1) == "+"_L1)
			{
				stepPositive = true;
				step = step.mid(1).trimmed();
			}
			else if(step.left(1) == "-"_L1)
			{
				stepPositive = false;
				step = step.mid(1).trimmed();
			}
			else stepPositive = true;
		}
		else
		{
			step = "1";
			stepPositive = true;
		}

		QString variable = fieldMap["initExpression"_L1].string().mid(0,fieldMap["initExpression"_L1].string().indexOf("="_L1)).trimmed();
		QString endExpr = variable+ " <= "_L1 + fieldMap["toExpression"_L1].string().trimmed();

		if( fieldMap["stepExpression"_L1].found() )
		{
			bool isConstant;
			step = processConstant(step,&isConstant);
			if( !isConstant )
				mistake( MicrobeApp::NonConstantStep );
		}

		SourceLineList tempList;
		tempList << SourceLineMicrobe( fieldMap["initExpression"_L1].string(), nullptr, -1 );

		m_pPic->Sfor( parseWithChild( fieldMap["code"_L1].bracedCode() ), parseWithChild( tempList ), endExpr, variable, step, stepPositive );
	}
	else if( name == "alias"_L1 )
	{
		// It is important to get this the right way round!
		// The alias should be the key since two aliases could
		// point to the same name.

		QString alias = fieldMap["alias"_L1].string().trimmed();
		QString dest = fieldMap["dest"_L1].string().trimmed();

		// Check to see whether or not we've already aliased it...
// 		if ( mb->alias(alias) != alias )
// 			mistake( MicrobeApp::AliasRedefined );
// 		else
			mb->addAlias( alias, dest );
	}
	else if( name == "increment"_L1 )
	{
		QString variableName = fieldMap["variable"_L1].string();

		if ( !mb->isVariableKnown( variableName ) )
			mistake( MicrobeApp::UnknownVariable );
		else if ( !mb->variable( variableName ).isWritable() )
			mistake( MicrobeApp::ReadOnlyVariable, variableName );
		else
			m_pPic->SincVar( variableName );
	}
	else if( name == "decrement"_L1 )
	{
		QString variableName = fieldMap["variable"_L1].string();

		if ( !mb->isVariableKnown( variableName ) )
			mistake( MicrobeApp::UnknownVariable );
		else if ( !mb->variable( variableName ).isWritable() )
			mistake( MicrobeApp::ReadOnlyVariable, variableName );
		else
			m_pPic->SdecVar( variableName );
	}
	else if( name == "rotateleft"_L1 )
	{
		QString variableName = fieldMap["variable"_L1].string();

		if ( !mb->isVariableKnown( variableName ) )
			mistake( MicrobeApp::UnknownVariable );
		else if ( !mb->variable( variableName ).isWritable() )
			mistake( MicrobeApp::ReadOnlyVariable, variableName );
		else
			m_pPic->SrotlVar( variableName );
	}
	else if( name == "rotateright"_L1 )
	{
		QString variableName = fieldMap["variable"_L1].string();

		if ( !mb->isVariableKnown( variableName ) )
			mistake( MicrobeApp::UnknownVariable );
		else if ( !mb->variable( variableName ).isWritable() )
			mistake( MicrobeApp::ReadOnlyVariable, variableName );
		else
			m_pPic->SrotrVar( variableName );
	}
	else if( name == "asm"_L1 )
	{
		m_pPic->Sasm( SourceLineMicrobe::toStringList( fieldMap["code"_L1].bracedCode() ).join("\n"_L1) );
	}
	else if( name == "delay"_L1 )
	{
		// This is one of the rare occasions that the number will be bigger than a byte,
		// so suppressNumberTooBig must be used
		bool isConstant;
		QString delay = processConstant(fieldMap["expression"_L1].string(),&isConstant,true);
		if (!isConstant)
			mistake( MicrobeApp::NonConstantDelay );
// 		else m_pPic->Sdelay( fieldMap["expression"_L1].string(), ""_L1);
		else
		{
			// TODO We should use the "delay"_L1 string returned by processConstant - not the expression (as, e.g. 2*3 won't be ok)
			int length_ms = literalToInt( fieldMap["expression"_L1].string() );
			if ( length_ms >= 0 )
				m_pPic->Sdelay( length_ms * 1000 ); // Pause the delay length in microseconds
			else
				mistake( MicrobeApp::NonConstantDelay );
		}
	}
	else if ( name == "keypad"_L1 || name == "sevenseg"_L1 )
	{
		//QStringList pins = QStringList::split( ' ', fieldMap["pinlist"_L1].string() );
        QStringList pins = fieldMap["pinlist"_L1].string().split(' ', Qt::SkipEmptyParts);
		QString variableName = fieldMap["name"_L1].string();

		if ( mb->isVariableKnown( variableName ) )
		{
			mistake( MicrobeApp::VariableRedefined, variableName );
			return;
		}

		PortPinList pinList;

		QStringList::iterator end = pins.end();
		for ( QStringList::iterator it = pins.begin(); it != end; ++it )
		{
			PortPin portPin = m_pPic->toPortPin(*it);
			if ( portPin.pin() == -1 )
			{
				// Invalid port/pin
				//TODO mistake
				return;
			}
			pinList << portPin;
		}

		if ( name == "keypad"_L1 )
		{
			Variable v( Variable::keypadType, variableName );
			v.setPortPinList( pinList );
			mb->addVariable( v );
		}

		else // name == "sevenseg"
		{
			if ( pinList.size() != 7 )
				mistake( MicrobeApp::InvalidPinMapSize );
			else
			{
				Variable v( Variable::sevenSegmentType, variableName );
				v.setPortPinList( pinList );
				mb->addVariable( v );
			}
		}
	}
}


void Parser::mistake( MicrobeApp::MistakeType type, const QString & context )
{
	mb->compileError( type, context, m_currentSourceLine );
}


Statement::Statement() : code(NULL) {
}

// static function
QStringList Statement::tokenise(const QString &line)
{
	QStringList result;
	QString current;
	int count = 0;

	for(int i = 0; i < int(line.length()); i++)
	{
		QChar nextChar = line[i];
		if( nextChar.isSpace() )
		{
			if( count > 0 )
			{
				result.append(current);
				current = "";
				count = 0;
			}
		}
		else if( nextChar == '=' )
		{
			if( count > 0 ) result.append(current);
			current = "";
			count = 0;
			result.append("="_L1);
		}
		else if( nextChar == '{' )
		{
			if( count > 0 ) result.append(current);
			current = "";
			count = 0;
			result.append("{"_L1);
		}
		else
		{
			count++;
			current.append(nextChar);
		}
	}
	if( count > 0 ) result.append(current);
	return result;
}

int Parser::doArithmetic(int lvalue, int rvalue, Expression::Operation op)
{
	switch(op)
	{
		case Expression::noop: return 0;
		case Expression::addition: return lvalue + rvalue;
		case Expression::subtraction: return lvalue - rvalue;
		case Expression::multiplication: return lvalue * rvalue;
		case Expression::division: return lvalue / rvalue;
		case Expression::exponent: return lvalue ^ rvalue;
		case Expression::equals: return lvalue == rvalue;
		case Expression::notequals: return !(lvalue == rvalue);
		case Expression::bwand: return lvalue & rvalue;
		case Expression::bwor: return lvalue | rvalue;
		case Expression::bwxor: return lvalue ^ rvalue;
		case Expression::bwnot: return !rvalue;
		case Expression::le: return lvalue <= rvalue;
		case Expression::ge: return lvalue >= rvalue;
		case Expression::lt: return lvalue < rvalue;
		case Expression::gt: return lvalue > rvalue;

		case Expression::pin:
		case Expression::notpin:
		case Expression::function:
		case Expression::divbyzero:
		case Expression::read_keypad:
			// Not applicable actions.
			break;
	}
	return -1;
}

bool Parser::isLiteral( const QString &text )
{
	bool ok;
	literalToInt( text, & ok );
	return ok;
}

/*
Literal's in form:
-> 321890
-> 021348
-> 0x3C
-> b'0100110'
-> 0101001b
-> h'43A'
-> 2Ah

Everything else is non-literal...
*/
int Parser::literalToInt( const QString &literal, bool * ok )
{
	bool temp;
	if ( !ok )
		ok = & temp;
	*ok = true;

	int value = -1;

	// Note when we use toInt, we don't have to worry about checking
	// that literal.mid() is convertible, as toInt returns this in ok anyway.

	// Try binary first, of form b'n...n'
	if( literal.left(2) == "b'"_L1 && literal.right(1) == "'"_L1 )
	{
		value = literal.mid(2,literal.length() - 3).toInt(ok,2);
		return *ok ? value : -1;
	}

	// Then try hex of form h'n...n'
	if( literal.left(2) == "h'"_L1 && literal.right(1) == "'"_L1 )
	{
		value = literal.mid(2,literal.length() - 3).toInt(ok,16);
		return *ok ? value : -1;
	}

	// otherwise, let QString try and convert it
	// base 0 == automatic base guessing
	value = literal.toInt( ok, 0 );
	return *ok ? value : -1;
}


void Parser::compileConditionalExpression( const QString & expression, Code * ifCode, Code * elseCode ) const
{
	///HACK ///TODO this is a little improper, I don't think we should be using the pic that called us...

	Expression( m_pPic, mb, m_currentSourceLine, false ).compileConditional(expression,ifCode,elseCode);
}


QString Parser::processConstant(const QString &expression, bool * isConstant, bool suppressNumberTooBig) const
{
	return Expression( m_pPic, mb, m_currentSourceLine, suppressNumberTooBig ).processConstant(expression, isConstant);
}
//END class Parser



//BEGIN class Field
Field::Field()
{
	m_type = None;
	m_compulsory = false;
}


Field::Field( Type type, const QString & key )
{
	m_type = type;
	m_compulsory = false;
	m_key = key;
}


Field::Field( Type type, const QString & key, const QString & string, bool compulsory )
{
	m_type = type;
	m_compulsory = compulsory;
	m_key = key;
	m_string = string;
}
//END class Field



//BEGIN class OutputField
OutputField::OutputField()
{
	m_found = false;
}


OutputField::OutputField( const SourceLineList & bracedCode )
{
	m_bracedCode = bracedCode;
	m_found = true;
}

OutputField::OutputField( const QString & string/*, int lineNumber*/ )
{
	m_string = string;
	m_found = true;
}
//END class OutputField



#if 0
// Second pass

		else if( firstToken == "include"_L1 )
		{
			// only cope with 'sane' strings a.t.m.
			// e.g. include "filename.extension"
			QString filename = (*sit).content.mid( (*sit).content.find("\""_L1) ).trimmed();
			// don't strip whitespace from within quotes as you
			// can have filenames composed entirely of spaces (kind of weird)...
			// remove quotes.
			filename = filename.mid(1);
			filename = filename.mid(0,filename.length()-1);
			QFile includeFile(filename);
			if( includeFile.open(QIODevice::ReadOnly) )
			{
				QTextStream stream( &includeFile );
        			QStringList includeCode;
				while( !stream.atEnd() )
				{
					includeCode += stream.readLine();
				}
				///TODO make includes work
				//output += parse(includeCode);
				includeFile.close();
    			}
    			else
    			mistake( MicrobeApp::UnopenableInclude, filename );
 		}
#endif


