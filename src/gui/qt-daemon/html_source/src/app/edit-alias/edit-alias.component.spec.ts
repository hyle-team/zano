import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { EditAliasComponent } from './edit-alias.component';

describe('EditAliasComponent', () => {
  let component: EditAliasComponent;
  let fixture: ComponentFixture<EditAliasComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ EditAliasComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(EditAliasComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
