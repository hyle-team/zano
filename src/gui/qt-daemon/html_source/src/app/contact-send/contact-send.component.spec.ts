import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { ContactSendComponent } from './contact-send.component';

describe('ContactSendComponent', () => {
  let component: ContactSendComponent;
  let fixture: ComponentFixture<ContactSendComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ ContactSendComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ContactSendComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
